// admin.cpp - with proper concurrency control
#include "platform.h"
#include <thread>
#include <vector>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <atomic>

using namespace std;

// ------------------- Data Structures -------------------
class Movie
{
public:
    string name;
    string lang;
    int rating;
    int cost;
};

static vector<Movie> movieList;
static unordered_map<string, vector<vector<int>>> seatDB; // key = "movie_date"

// ------------------- Mutex Declarations -------------------
static mutex movieMutex;                                    // For movie list operations
static mutex fileMutex;                                     // For file I/O operations
static unordered_map<string, mutex> showMutexes;          // Per-show mutexes
static mutex showMutexMapMutex;                            // For accessing showMutexes map
static atomic<bool> running{true};

// ------------------- File I/O Helpers -------------------
void loadMovies()
{
    lock_guard<mutex> lock(fileMutex);
    ifstream fin("movies.txt");
    movieList.clear();
    if (!fin.is_open())
        return;

    string line;
    while (getline(fin, line))
    {
        istringstream iss(line);
        Movie m;
        if (iss >> m.name >> m.lang >> m.rating >> m.cost)
        {
            movieList.push_back(m);
        }
    }
}

void saveMovies()
{
    lock_guard<mutex> lock(fileMutex);
    ofstream fout("movies.txt");
    for (auto &m : movieList)
    {
        fout << m.name << " " << m.lang << " " << m.rating << " " << m.cost << "\n";
    }
}

void saveSeats(const string &key, const vector<vector<int>> &hall)
{
    lock_guard<mutex> lock(fileMutex);
    ofstream fout("seats_" + key + ".txt");
    for (int i = 0; i < 9; i++)
    {
        for (int j = 0; j < 9; j++)
        {
            if (hall[i][j] == -1)
                fout << i << " " << j << "\n";
        }
    }
}

void loadSeats(const string &key)
{
    lock_guard<mutex> lock(fileMutex);
    vector<vector<int>> hall(9, vector<int>(9, 1));
    ifstream fin("seats_" + key + ".txt");
    int r, c;
    while (fin >> r >> c)
    {
        if (r >= 0 && r < 9 && c >= 0 && c < 9)
            hall[r][c] = -1;
    }
    seatDB[key] = hall;
}

// ------------------- Mutex Helper for Per-Show Locking -------------------
mutex& getShowMutex(const string& key)
{
    lock_guard<mutex> mapLock(showMutexMapMutex);
    return showMutexes[key];  // Creates mutex if doesn't exist
}

// ------------------- Date Validation -------------------
bool validDate(const string &date)
{
    // expects YYYYMMDD format
    time_t now = time(0);

    int y = stoi(date.substr(0, 4));
    int m = stoi(date.substr(4, 2));
    int d = stoi(date.substr(6, 2));

    tm input{};
    input.tm_year = y - 1900;
    input.tm_mon = m - 1;
    input.tm_mday = d;

    time_t inputTime = mktime(&input);
    double diff = difftime(inputTime, now) / (60 * 60 * 24);

    return (diff >= 0 && diff <= 3); // must be today or within 3 days
}

// ------------------- Admin Menu -------------------
void adminMenu()
{
    while (true)
    {
        cout << "\n=== Admin Menu ===\n";
        cout << "1. List Movies\n2. Add Movie\n3. Remove Movie\n4. Exit Admin Menu\n";
        cout << "Choice: ";

        int ch;
        cin >> ch;
        cout << endl;

        if (ch == 1)
        {
            lock_guard<mutex> lock(movieMutex);
            for (size_t i = 0; i < movieList.size(); i++)
            {
                cout << i + 1 << ". " << movieList[i].name << " (" << movieList[i].lang << ") "
                     << "Rating:" << movieList[i].rating << " Cost:" << movieList[i].cost << "\n";
            }
        }
        else if (ch == 2)
        {
            Movie m;
            cout << "Enter name: ";
            cin >> m.name;
            cout << "Enter language: ";
            cin >> m.lang;
            cout << "Enter rating: ";
            cin >> m.rating;
            cout << "Enter base ticket price: ";
            cin >> m.cost;

            lock_guard<mutex> lock(movieMutex);
            movieList.push_back(m);
            saveMovies();
        }
        else if (ch == 3)
        {
            int idx;
            cout << "Enter index to remove: ";
            cin >> idx;
            lock_guard<mutex> lock(movieMutex);
            if (idx >= 1 && idx <= (int)movieList.size())
            {
                movieList.erase(movieList.begin() + idx - 1);
                saveMovies();
            }
        }
        else if (ch == 4)
            break;
    }
}

// ------------------- Client Handler -------------------
bool recvAll(SOCKET s, char *buffer, int length)
{
    int total = 0;
    while (total < length)
    {
        int n = recv(s, buffer + total, length - total, 0);
        if (n <= 0)
            return false;
        total += n;
    }
    return true;
}

void handleClient(SOCKET c)
{
    while (true)
    {
        char req[8] = {0};
        int n = recv(c, req, sizeof(req) - 1, 0);
        if (n <= 0)
        {
            closesocket(c);
            return;
        }

        if (req[0] == '1')
        {
            // Send movie list
            lock_guard<mutex> lock(movieMutex);
            int sz = movieList.size();
            send(c, (char *)&sz, sizeof(sz), 0);
            for (auto &m : movieList)
            {
                int len = m.name.size();
                send(c, (char *)&len, sizeof(len), 0);
                send(c, m.name.c_str(), len, 0);

                len = m.lang.size();
                send(c, (char *)&len, sizeof(len), 0);
                send(c, m.lang.c_str(), len, 0);

                send(c, (char *)&m.rating, sizeof(m.rating), 0);
                send(c, (char *)&m.cost, sizeof(m.cost), 0);
            }
        }
        else if (req[0] == '2')
        {
            // Send hall matrix + dynamic price
            int len;
            if (!recvAll(c, (char *)&len, sizeof(len)))
                break;
            string movie(len, '\0');
            if (!recvAll(c, &movie[0], len))
                break;

            int dlen;
            if (!recvAll(c, (char *)&dlen, sizeof(dlen)))
                break;
            string date(dlen, '\0');
            if (!recvAll(c, &date[0], dlen))
                break;

            string key = movie + "_" + date;
            if (!validDate(date))
            {
                int ok = 0;
                send(c, (char *)&ok, sizeof(ok), 0);
                continue;
            }

            // Get per-show mutex and lock it
            mutex& showMutex = getShowMutex(key);
            lock_guard<mutex> showLock(showMutex);

            // Load seats if not already loaded (now thread-safe)
            if (seatDB.find(key) == seatDB.end())
            {
                loadSeats(key);
            }

            auto &currHall = seatDB[key];
            int total = 81, booked = 0;
            for (int i = 0; i < 9; i++)
                for (int j = 0; j < 9; j++)
                    if (currHall[i][j] == -1)
                        booked++;

            double occ = (double)booked / total;
            int base = 100; // fallback price
            
            // Find base price (need to lock movie list)
            {
                lock_guard<mutex> movieLock(movieMutex);
                for (auto &m : movieList)
                    if (m.name == movie)
                        base = m.cost;
            }

            int dynPrice = base * (1 + occ);

            int ok = 1;
            send(c, (char *)&ok, sizeof(ok), 0);
            for (int i = 0; i < 9; i++)
                send(c, reinterpret_cast<char *>(currHall[i].data()), sizeof(int) * 9, 0);
            send(c, (char *)&dynPrice, sizeof(dynPrice), 0);
        }

        else if (req[0] == '3')
        {
            // Book seats
            int len;
            if (!recvAll(c, (char *)&len, sizeof(len)))
                break;
            string key(len, '\0');
            if (!recvAll(c, &key[0], len))
                break;

            // Get per-show mutex and lock it
            mutex& showMutex = getShowMutex(key);
            lock_guard<mutex> showLock(showMutex);

            // Load seats if not already loaded (now thread-safe)
            if (seatDB.find(key) == seatDB.end())
            {
                loadSeats(key);
            }
            auto &currHall = seatDB[key];

            int seat[10];
            memset(seat, -1, sizeof(seat));
            if (!recvAll(c, (char *)seat, sizeof(seat)))
                break;

            bool success = true;
            vector<int> bookedSeats; // Track successfully booked seats for rollback

            // Try to book all requested seats
            for (int i = 0; i < 10; i++)
            {
                if (seat[i] >= 0)
                {
                    int row = (seat[i] - 1) / 9;
                    int col = (seat[i] - 1) % 9;
                    if (row >= 0 && row < 9 && col >= 0 && col < 9)
                    {
                        if (currHall[row][col] == 1)
                        {
                            currHall[row][col] = -1;
                            bookedSeats.push_back(seat[i]);
                        }
                        else
                        {
                            success = false;
                            break;
                        }
                    }
                }
            }

            // If booking failed, rollback all changes
            if (!success)
            {
                for (int seatNum : bookedSeats)
                {
                    int row = (seatNum - 1) / 9;
                    int col = (seatNum - 1) % 9;
                    currHall[row][col] = 1; // Restore to available
                }
            }

            // Save the updated seat configuration
            saveSeats(key, currHall);
            
            int result = success ? 1 : 0;
            send(c, (char *)&result, sizeof(result), 0);
        }
        else if (req[0] == '4')
        {
            break;
        }
    }
    closesocket(c);
}

// ------------------- Server -------------------
void client_ops_server()
{
    SOCKET ls = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in a{};
    a.sin_family = AF_INET;
    a.sin_port = htons(12347);
    a.sin_addr.s_addr = INADDR_ANY;
    bind(ls, (sockaddr *)&a, sizeof(a));
    listen(ls, 16);
    cout << "[ADMIN] Client-ops server listening on port 12347...\n";

    while (running)
    {
        SOCKET c = accept(ls, nullptr, nullptr);
        if (c == INVALID_SOCKET)
            continue;
        thread(handleClient, c).detach();
    }
    closesocket(ls);
}

// ------------------- Main -------------------
int main()
{
    if (!winsock_startup())
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }
    loadMovies();
    thread tOps(client_ops_server);
    this_thread::sleep_for(std::chrono::milliseconds(100));
    adminMenu();
    running = false;
    tOps.join();
    winsock_cleanup();
    return 0;
}
