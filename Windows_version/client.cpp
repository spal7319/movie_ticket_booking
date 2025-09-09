// client.cpp
#include "platform.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <cstdlib>
#include <windows.h>
#include<unordered_map>

using namespace std;

string currentUser;
string savedPassword;
int currentBalance = 0;


// both will be updated at the time of requestSeats()
string lastMovie;
string lastDate;
int lastPrice = 0;

// -------------------- File I/O for Users --------------------
bool loadUser(const string &username, string &password, int &balance)
{
    ifstream fin("users.txt");
    string u, p;
    int b;
    while (fin >> u >> p >> b)
    {
        if (u == username)
        {
            password = p;
            balance = b;
            return true;
        }
    }
    return false;
}

void saveUser(const string &username, const string &password, int balance)
{
    ifstream fin("users.txt");
    vector<tuple<string, string, int>> users;
    string u, p;
    int b;
    bool found = false;
    while (fin >> u >> p >> b)
    {
        if (u == username)
        {
            users.push_back({u, password, balance});
            found = true;
        }
        else
        {
            users.push_back({u, p, b});
        }
    }
    fin.close();
    if (!found)
    {
        users.push_back({username, password, balance});
    }
    ofstream fout("users.txt");
    for (auto &x : users)
    {
        fout << get<0>(x) << " " << get<1>(x) << " " << get<2>(x) << "\n";
    }
}

// -------------------- Signup/Login --------------------
bool signup()
{
    string username, password;
    cout << "Enter new username: ";
    cin >> username;
    cout << "Enter password: ";
    cin >> password;
    currentUser = username;
    savedPassword = password;
    currentBalance = 2000; // default starting balance
    saveUser(username, password, currentBalance);
    cout << "Signup successful. Starting balance = " << currentBalance << "\n";
    return true;
}

bool login()
{
    string username, password;
    cout << "Enter username: ";
    cin >> username;
    cout << "Enter password: ";
    cin >> password;
    string storedPass;
    int bal;
    if (loadUser(username, storedPass, bal) && storedPass == password)
    {
        currentUser = username;
        savedPassword = password;
        currentBalance = bal;
        cout << "Login successful. Balance=" << bal << "\n";
        return true;
    }
    cout << "Login failed.\n";
    return false;
}

// -------------------- Networking Helpers --------------------
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

void requestMovies(SOCKET sock)
{
    char req = '1';
    send(sock, &req, 1, 0);
    int sz;
    recvAll(sock, (char *)&sz, sizeof(sz));

    cout << "=== Movies ===\n";
    for (int i = 0; i < sz; i++)
    {
        // --- receive name ---
        int len;
        recvAll(sock, (char *)&len, sizeof(len));
        string name(len, '\0');
        recvAll(sock, &name[0], len);

        // --- receive lang ---
        recvAll(sock, (char *)&len, sizeof(len));
        string lang(len, '\0');
        recvAll(sock, &lang[0], len);

        // --- receive rating & cost ---
        int rating;
        int cost;
        recvAll(sock, (char *)&rating, sizeof(rating));
        recvAll(sock, (char *)&cost, sizeof(cost));

        cout << i + 1 << ". " << name << " (" << lang << ") Rating:" << rating /*<< " Cost:" << cost*/ << "\n";
    }
}

void requestSeats(SOCKET sock)
{
    cout << "Enter movie name: ";
    cin >> lastMovie;
    cout << "\nEnter date (YYYYMMDD): ";
    cin >> lastDate;

    char req = '2';
    send(sock, &req, 1, 0);

    int len = lastMovie.size();
    send(sock, (char *)&len, sizeof(len), 0);
    send(sock, lastMovie.c_str(), len, 0);

    int dlen = lastDate.size();
    send(sock, (char *)&dlen, sizeof(dlen), 0);
    send(sock, lastDate.c_str(), dlen, 0);

    int ok;
    if (!recvAll(sock, (char *)&ok, sizeof(ok)) || ok == 0)
    {
        cout << "Invalid date. Can only book within 3 days.\n";
        return;
    }

    int hall[9][9];
    recvAll(sock, (char *)hall, sizeof(hall));

    int price;
    recvAll(sock, (char *)&price, sizeof(price));
    lastPrice = price;

    cout << "=== Seats for " << lastMovie << " on " << lastDate << " ===\n";
    cout << "(price per seat = " << price << ")\n";

    for (int row = 0; row < 9; row++)
    {
        for (int col = 0; col < 9; col++)
        {
            int seatNum = row * 9 + col + 1;
            if (hall[row][col] == 1)
            {
                if(seatNum >= 1 && 9 >= seatNum)
                {
                    cout << seatNum << "  ";
                }
                else
                {
                    cout << seatNum << " ";
                }
            }
                
            else
            {
                cout << "*  ";
            }
        }
        cout << "\n";
    }
}

void bookSeats(SOCKET sock)
{
    if (lastMovie.empty() || lastDate.empty())
    {
        cout << "Please view seats first (option 2) before booking.\n";
        return;
    }
    string key = lastMovie + "_" + lastDate;

    int n;
    cout << "How many seats? ";
    cin >> n;

    int reqBalance = n * lastPrice;
    if (currentBalance < reqBalance)
    {
        cout << "Your balance is low, seats can't be booked\n";
        return;
    }

    char req = '3';
    send(sock, &req, 1, 0);

    // send key first
    int len = key.size();
    send(sock, (char *)&len, sizeof(len), 0);
    send(sock, key.c_str(), len, 0);

    int seats[10];
    for (int i = 0; i < 10; i++) seats[i] = -1;
    for (int i = 0; i < n; i++)
    {
        cout << "Enter seat number: ";
        cin >> seats[i];
    }

    // password check
    bool flag = false;
    while (!flag)
    {
        string password;
        cout << "\nEnter your password: ";
        cin >> password;
        if (savedPassword == password)
            flag = true;
        else
            cout << "\nPassword Mismatched! Try again\n";
    }

    send(sock, (char *)seats, sizeof(seats), 0);
    cout << "Booking request sent.\n";

    int result;
    if (!recvAll(sock, (char *)&result, sizeof(result)))
    {
        cerr << "Failed to get booking confirmation.\n";
        return;
    }

    if (result == 1)
    {
        currentBalance -= n * lastPrice;
        saveUser(currentUser, savedPassword, currentBalance);

        cout << "\nBooking successful!\n";
        cout << "Wallet updated. New balance=" << currentBalance << "\n";
    }
    else
    {
        cout << "Booking failed! One or more seats were already booked.\n";
        cout << "Check the seat numbers which are not marked with '*' and try again\n";
    }
}


// -------------------- Main Menu --------------------
int main()
{
    if (!winsock_startup())
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }
    int choice;
    cout << "1. Signup\n2. Login\n"
         << endl;
    cout << "Enter Choice: ";

    cin >> choice;

    cout << endl;

    if (choice == 1)
        signup();

    else if (choice == 2)
    {
        while (!login())
            ;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(12347);
    serv.sin_addr.s_addr = inet_addr("127.0.0.1");
    connect(s, (sockaddr *)&serv, sizeof(serv));

    while (true)
    {
        cout << "\n=== Client Menu ===\n";

        cout << endl;

        cout << "1. List Movies\n\n2. Show Seats\n\n3. Book Seats\n\n4. Exit\n\n";

        cout << "Choice: ";
        int ch;
        cin >> ch;

        cout << endl;

        if (ch == 1)
            requestMovies(s);
        else if (ch == 2)
            requestSeats(s);
        else if (ch == 3)
            bookSeats(s);
        else
            break;
    }

    closesocket(s);
    winsock_cleanup();
    return 0;
}
