#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "structures.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <mutex>

class FileManager {
private:
    static std::mutex file_mutex;
    std::string data_dir;
    
    std::vector<std::string> split(const std::string& str, char delimiter);
    std::string getCurrentTime();
    
public:
    FileManager(const std::string& data_directory = "data");
    
    // User operations
    std::vector<User> loadUsers();
    bool saveUsers(const std::vector<User>& users);
    bool authenticateUser(const std::string& username, const std::string& password, User& user);
    bool addUser(const User& user);
    
    // Movie operations
    std::vector<Movie> loadMovies();
    bool saveMovies(const std::vector<Movie>& movies);
    bool addMovie(const Movie& movie);
    bool updateMovie(const Movie& movie);
    Movie getMovieById(int movie_id);
    
    // Show operations
    std::vector<Show> loadShows();
    bool saveShows(const std::vector<Show>& shows);
    bool addShow(const Show& show);
    bool updateShow(const Show& show);
    std::vector<Show> getShowsByMovieId(int movie_id);
    Show getShowById(int show_id);
    
    // Booking operations
    std::vector<Booking> loadBookings();
    bool saveBookings(const std::vector<Booking>& bookings);
    bool addBooking(const Booking& booking);
    std::vector<Booking> getBookingsByUserId(int user_id);
    
    // Seat operations
    std::vector<Seat> loadSeats();
    bool saveSeats(const std::vector<Seat>& seats);
    bool initializeSeatsForShow(int show_id, int total_seats);
    std::vector<Seat> getSeatsByShowId(int show_id);
    bool bookSeats(int show_id, const std::vector<int>& seat_numbers, int user_id);
    
    // Utility functions
    bool createDataDirectory();
    int getNextId(const std::string& filename);
};

// Implementation
std::mutex FileManager::file_mutex;

FileManager::FileManager(const std::string& data_directory) : data_dir(data_directory) {
    createDataDirectory();
}

bool FileManager::createDataDirectory() {
    // Create data directory if it doesn't exist
    #ifdef _WIN32
        system(("mkdir " + data_dir + " 2>nul").c_str());
    #else
        system(("mkdir -p " + data_dir).c_str());
    #endif
    return true;
}

std::vector<std::string> FileManager::split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

std::string FileManager::getCurrentTime() {
    time_t now = time(0);
    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H:%M", localtime(&now));
    return std::string(buffer);
}

int FileManager::getNextId(const std::string& filename) {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::ifstream file(data_dir + "/" + filename);
    int max_id = 0;
    std::string line;
    
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto tokens = split(line, '|');
            if (!tokens.empty()) {
                int id = std::stoi(tokens[0]);
                max_id = std::max(max_id, id);
            }
        }
    }
    return max_id + 1;
}

// User operations
std::vector<User> FileManager::loadUsers() {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::vector<User> users;
    std::ifstream file(data_dir + "/users.txt");
    std::string line;
    
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto tokens = split(line, '|');
            if (tokens.size() >= 4) {
                User user;
                user.user_id = std::stoi(tokens[0]);
                user.username = tokens[1];
                user.password = tokens[2];
                user.user_type = (tokens[3] == "ADMIN") ? ADMIN : CLIENT;
                users.push_back(user);
            }
        }
    }
    return users;
}

bool FileManager::saveUsers(const std::vector<User>& users) {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::ofstream file(data_dir + "/users.txt");
    if (!file.is_open()) return false;
    
    for (const auto& user : users) {
        file << user.user_id << "|" << user.username << "|" << user.password << "|"
             << (user.user_type == ADMIN ? "ADMIN" : "CLIENT") << "\n";
    }
    return true;
}

bool FileManager::authenticateUser(const std::string& username, const std::string& password, User& user) {
    auto users = loadUsers();
    for (const auto& u : users) {
        if (u.username == username && u.password == password) {
            user = u;
            return true;
        }
    }
    return false;
}

bool FileManager::addUser(const User& user) {
    auto users = loadUsers();
    users.push_back(user);
    return saveUsers(users);
}

// Movie operations
std::vector<Movie> FileManager::loadMovies() {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::vector<Movie> movies;
    std::ifstream file(data_dir + "/movies.txt");
    std::string line;
    
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto tokens = split(line, '|');
            if (tokens.size() >= 5) {
                Movie movie;
                movie.movie_id = std::stoi(tokens[0]);
                movie.movie_name = tokens[1];
                movie.duration = std::stoi(tokens[2]);
                movie.price = std::stod(tokens[3]);
                movie.status = (tokens[4] == "ACTIVE") ? ACTIVE : INACTIVE;
                movies.push_back(movie);
            }
        }
    }
    return movies;
}

bool FileManager::saveMovies(const std::vector<Movie>& movies) {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::ofstream file(data_dir + "/movies.txt");
    if (!file.is_open()) return false;
    
    for (const auto& movie : movies) {
        file << movie.movie_id << "|" << movie.movie_name << "|" << movie.duration << "|"
             << movie.price << "|" << (movie.status == ACTIVE ? "ACTIVE" : "INACTIVE") << "\n";
    }
    return true;
}

bool FileManager::addMovie(const Movie& movie) {
    auto movies = loadMovies();
    movies.push_back(movie);
    return saveMovies(movies);
}

Movie FileManager::getMovieById(int movie_id) {
    auto movies = loadMovies();
    for (const auto& movie : movies) {
        if (movie.movie_id == movie_id) {
            return movie;
        }
    }
    return Movie();
}

// Show operations
std::vector<Show> FileManager::loadShows() {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::vector<Show> shows;
    std::ifstream file(data_dir + "/shows.txt");
    std::string line;
    
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto tokens = split(line, '|');
            if (tokens.size() >= 6) {
                Show show;
                show.show_id = std::stoi(tokens[0]);
                show.movie_id = std::stoi(tokens[1]);
                show.show_date = tokens[2];
                show.show_time = tokens[3];
                show.total_seats = std::stoi(tokens[4]);
                show.available_seats = std::stoi(tokens[5]);
                shows.push_back(show);
            }
        }
    }
    return shows;
}

bool FileManager::saveShows(const std::vector<Show>& shows) {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::ofstream file(data_dir + "/shows.txt");
    if (!file.is_open()) return false;
    
    for (const auto& show : shows) {
        file << show.show_id << "|" << show.movie_id << "|" << show.show_date << "|"
             << show.show_time << "|" << show.total_seats << "|" << show.available_seats << "\n";
    }
    return true;
}

bool FileManager::addShow(const Show& show) {
    auto shows = loadShows();
    shows.push_back(show);
    if (saveShows(shows)) {
        // Initialize seats for the new show
        return initializeSeatsForShow(show.show_id, show.total_seats);
    }
    return false;
}

Show FileManager::getShowById(int show_id) {
    auto shows = loadShows();
    for (const auto& show : shows) {
        if (show.show_id == show_id) {
            return show;
        }
    }
    return Show();
}

std::vector<Show> FileManager::getShowsByMovieId(int movie_id) {
    auto shows = loadShows();
    std::vector<Show> movie_shows;
    for (const auto& show : shows) {
        if (show.movie_id == movie_id) {
            movie_shows.push_back(show);
        }
    }
    return movie_shows;
}

// Booking operations
std::vector<Booking> FileManager::loadBookings() {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::vector<Booking> bookings;
    std::ifstream file(data_dir + "/bookings.txt");
    std::string line;
    
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto tokens = split(line, '|');
            if (tokens.size() >= 7) {
                Booking booking;
                booking.booking_id = std::stoi(tokens[0]);
                booking.user_id = std::stoi(tokens[1]);
                booking.show_id = std::stoi(tokens[2]);
                booking.seats_booked = std::stoi(tokens[3]);
                booking.total_amount = std::stod(tokens[4]);
                booking.booking_time = tokens[5];
                booking.status = (tokens[6] == "CONFIRMED") ? CONFIRMED : CANCELLED;
                bookings.push_back(booking);
            }
        }
    }
    return bookings;
}

bool FileManager::saveBookings(const std::vector<Booking>& bookings) {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::ofstream file(data_dir + "/bookings.txt");
    if (!file.is_open()) return false;
    
    for (const auto& booking : bookings) {
        file << booking.booking_id << "|" << booking.user_id << "|" << booking.show_id << "|"
             << booking.seats_booked << "|" << booking.total_amount << "|" << booking.booking_time << "|"
             << (booking.status == CONFIRMED ? "CONFIRMED" : "CANCELLED") << "\n";
    }
    return true;
}

bool FileManager::addBooking(const Booking& booking) {
    auto bookings = loadBookings();
    bookings.push_back(booking);
    return saveBookings(bookings);
}

std::vector<Booking> FileManager::getBookingsByUserId(int user_id) {
    auto bookings = loadBookings();
    std::vector<Booking> user_bookings;
    for (const auto& booking : bookings) {
        if (booking.user_id == user_id) {
            user_bookings.push_back(booking);
        }
    }
    return user_bookings;
}

// Seat operations
std::vector<Seat> FileManager::loadSeats() {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::vector<Seat> seats;
    std::ifstream file(data_dir + "/seats.txt");
    std::string line;
    
    while (std::getline(file, line)) {
        if (!line.empty()) {
            auto tokens = split(line, '|');
            if (tokens.size() >= 4) {
                Seat seat;
                seat.show_id = std::stoi(tokens[0]);
                seat.seat_number = std::stoi(tokens[1]);
                seat.status = (tokens[2] == "AVAILABLE") ? AVAILABLE : BOOKED;
                seat.booked_by_user_id = std::stoi(tokens[3]);
                seats.push_back(seat);
            }
        }
    }
    return seats;
}

bool FileManager::saveSeats(const std::vector<Seat>& seats) {
    std::lock_guard<std::mutex> lock(file_mutex);
    std::ofstream file(data_dir + "/seats.txt");
    if (!file.is_open()) return false;
    
    for (const auto& seat : seats) {
        file << seat.show_id << "|" << seat.seat_number << "|"
             << (seat.status == AVAILABLE ? "AVAILABLE" : "BOOKED") << "|"
             << seat.booked_by_user_id << "\n";
    }
    return true;
}

bool FileManager::initializeSeatsForShow(int show_id, int total_seats) {
    auto seats = loadSeats();
    for (int i = 1; i <= total_seats; i++) {
        Seat seat(show_id, i);
        seats.push_back(seat);
    }
    return saveSeats(seats);
}

std::vector<Seat> FileManager::getSeatsByShowId(int show_id) {
    auto seats = loadSeats();
    std::vector<Seat> show_seats;
    for (const auto& seat : seats) {
        if (seat.show_id == show_id) {
            show_seats.push_back(seat);
        }
    }
    return show_seats;
}

bool FileManager::bookSeats(int show_id, const std::vector<int>& seat_numbers, int user_id) {
    auto seats = loadSeats();
    auto shows = loadShows();
    
    // Check if seats are available
    for (int seat_num : seat_numbers) {
        bool found = false;
        for (auto& seat : seats) {
            if (seat.show_id == show_id && seat.seat_number == seat_num) {
                if (seat.status == BOOKED) {
                    return false; // Seat already booked
                }
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    
    // Book the seats
    for (int seat_num : seat_numbers) {
        for (auto& seat : seats) {
            if (seat.show_id == show_id && seat.seat_number == seat_num) {
                seat.status = BOOKED;
                seat.booked_by_user_id = user_id;
                break;
            }
        }
    }
    
    // Update available seats count
    for (auto& show : shows) {
        if (show.show_id == show_id) {
            show.available_seats -= seat_numbers.size();
            break;
        }
    }
    
    return saveSeats(seats) && saveShows(shows);
}

#endif