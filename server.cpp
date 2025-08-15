#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "structures.h"
#include "file_manager.h"

#pragma comment(lib, "ws2_32.lib")

class MovieBookingServer {
private:
    SOCKET server_socket;
    FileManager* file_manager;
    int port;
    
    void initializeDefaultData() {
        // Create default admin user
        auto users = file_manager->loadUsers();
        if (users.empty()) {
            User admin(1, "admin", "admin123", ADMIN);
            User client(2, "user", "user123", CLIENT);
            file_manager->addUser(admin);
            file_manager->addUser(client);
            std::cout << "Default users created (admin/admin123, user/user123)\n";
        }
        
        // Create default movies
        auto movies = file_manager->loadMovies();
        if (movies.empty()) {
            Movie m1(1, "Avengers Endgame", 180, 150.0);
            Movie m2(2, "Spider-Man", 120, 120.0);
            Movie m3(3, "Batman", 140, 130.0);
            file_manager->addMovie(m1);
            file_manager->addMovie(m2);
            file_manager->addMovie(m3);
            std::cout << "Default movies created\n";
            
            // Create default shows
            Show s1(1, 1, "2025-08-16", "10:00", 50);
            Show s2(2, 1, "2025-08-16", "14:00", 50);
            Show s3(3, 2, "2025-08-16", "18:00", 50);
            file_manager->addShow(s1);
            file_manager->addShow(s2);
            file_manager->addShow(s3);
            std::cout << "Default shows created\n";
        }
    }
    
    std::string handleClientRequest(const std::string& request, int& user_id, UserType& user_type) {
        std::istringstream iss(request);
        std::string command;
        iss >> command;
        
        if (command == "LOGIN") {
            std::string username, password;
            iss >> username >> password;
            
            User user;
            if (file_manager->authenticateUser(username, password, user)) {
                user_id = user.user_id;
                user_type = user.user_type;
                return "LOGIN_SUCCESS|" + std::to_string(user.user_id) + "|" + 
                       (user.user_type == ADMIN ? "ADMIN" : "CLIENT");
            }
            return "LOGIN_FAILED";
        }
        
        if (user_id == 0) {
            return "NOT_AUTHENTICATED";
        }
        
        if (command == "VIEW_MOVIES") {
            auto movies = file_manager->loadMovies();
            std::string response = "MOVIES|";
            for (const auto& movie : movies) {
                if (movie.status == ACTIVE) {
                    response += std::to_string(movie.movie_id) + ":" + movie.movie_name + 
                               ":" + std::to_string(movie.duration) + ":" + 
                               std::to_string(movie.price) + ";";
                }
            }
            return response;
        }
        
        else if (command == "VIEW_SHOWS") {
            int movie_id;
            iss >> movie_id;
            
            auto shows = file_manager->getShowsByMovieId(movie_id);
            std::string response = "SHOWS|";
            for (const auto& show : shows) {
                response += std::to_string(show.show_id) + ":" + show.show_date + 
                           ":" + show.show_time + ":" + std::to_string(show.available_seats) + ";";
            }
            return response;
        }
        
        else if (command == "VIEW_SEATS") {
            int show_id;
            iss >> show_id;
            
            auto seats = file_manager->getSeatsByShowId(show_id);
            std::string response = "SEATS|";
            for (const auto& seat : seats) {
                response += std::to_string(seat.seat_number) + ":" + 
                           (seat.status == AVAILABLE ? "AVAILABLE" : "BOOKED") + ";";
            }
            return response;
        }
        
        else if (command == "BOOK_TICKETS") {
            int show_id, num_seats;
            iss >> show_id >> num_seats;
            
            std::vector<int> seat_numbers;
            for (int i = 0; i < num_seats; i++) {
                int seat_num;
                iss >> seat_num;
                seat_numbers.push_back(seat_num);
            }
            
            if (file_manager->bookSeats(show_id, seat_numbers, user_id)) {
                // Create booking record
                Show show = file_manager->getShowById(show_id);
                Movie movie = file_manager->getMovieById(show.movie_id);
                
                Booking booking;
                booking.booking_id = file_manager->getNextId("bookings.txt");
                booking.user_id = user_id;
                booking.show_id = show_id;
                booking.seats_booked = num_seats;
                booking.total_amount = movie.price * num_seats;
                booking.booking_time = "2025-08-15_" + std::to_string(time(nullptr));
                booking.status = CONFIRMED;
                
                file_manager->addBooking(booking);
                
                return "BOOKING_SUCCESS|" + std::to_string(booking.booking_id) + 
                       "|" + std::to_string(booking.total_amount);
            }
            return "BOOKING_FAILED";
        }
        
        else if (command == "MY_BOOKINGS") {
            auto bookings = file_manager->getBookingsByUserId(user_id);
            std::string response = "MY_BOOKINGS|";
            for (const auto& booking : bookings) {
                Show show = file_manager->getShowById(booking.show_id);
                Movie movie = file_manager->getMovieById(show.movie_id);
                response += std::to_string(booking.booking_id) + ":" + movie.movie_name + 
                           ":" + show.show_date + ":" + show.show_time + ":" + 
                           std::to_string(booking.seats_booked) + ":" + 
                           std::to_string(booking.total_amount) + ";";
            }
            return response;
        }
        
        // Admin commands
        else if (command == "ADD_MOVIE" && user_type == ADMIN) {
            std::string movie_name;
            int duration;
            double price;
            iss >> movie_name >> duration >> price;
            
            // Replace underscores with spaces in movie name
            for (auto& c : movie_name) {
                if (c == '_') c = ' ';
            }
            
            Movie movie;
            movie.movie_id = file_manager->getNextId("movies.txt");
            movie.movie_name = movie_name;
            movie.duration = duration;
            movie.price = price;
            movie.status = ACTIVE;
            
            if (file_manager->addMovie(movie)) {
                return "MOVIE_ADDED_SUCCESS";
            }
            return "MOVIE_ADD_FAILED";
        }
        
        else if (command == "ADD_SHOW" && user_type == ADMIN) {
            int movie_id, total_seats;
            std::string date, time;
            iss >> movie_id >> date >> time >> total_seats;
            
            Show show;
            show.show_id = file_manager->getNextId("shows.txt");
            show.movie_id = movie_id;
            show.show_date = date;
            show.show_time = time;
            show.total_seats = total_seats;
            show.available_seats = total_seats;
            
            if (file_manager->addShow(show)) {
                return "SHOW_ADDED_SUCCESS";
            }
            return "SHOW_ADD_FAILED";
        }
        
        else if (command == "ALL_BOOKINGS" && user_type == ADMIN) {
            auto bookings = file_manager->loadBookings();
            std::string response = "ALL_BOOKINGS|";
            for (const auto& booking : bookings) {
                Show show = file_manager->getShowById(booking.show_id);
                Movie movie = file_manager->getMovieById(show.movie_id);
                response += std::to_string(booking.booking_id) + ":" + 
                           std::to_string(booking.user_id) + ":" + movie.movie_name + 
                           ":" + show.show_date + ":" + show.show_time + ":" + 
                           std::to_string(booking.seats_booked) + ":" + 
                           std::to_string(booking.total_amount) + ";";
            }
            return response;
        }
        
        return "UNKNOWN_COMMAND";
    }
    
    void handleClient(SOCKET client_socket) {
        char buffer[1024];
        int user_id = 0;
        UserType user_type = CLIENT;
        
        std::cout << "Client connected\n";
        
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received <= 0) {
                break;
            }
            
            std::string request(buffer);
            std::string response = handleClientRequest(request, user_id, user_type);
            
            send(client_socket, response.c_str(), response.length(), 0);
        }
        
        closesocket(client_socket);
        std::cout << "Client disconnected\n";
    }
    
public:
    MovieBookingServer(int port = 8080) : port(port) {
        file_manager = new FileManager();
        initializeDefaultData();
    }
    
    ~MovieBookingServer() {
        delete file_manager;
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
        }
        WSACleanup();
    }
    
    bool start() {
        // Initialize Winsock
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return false;
        }
        
        // Create socket
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }
        
        // Setup server address
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        // Bind socket
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(server_socket);
            WSACleanup();
            return false;
        }
        
        // Listen for connections
        if (listen(server_socket, 5) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(server_socket);
            WSACleanup();
            return false;
        }
        
        std::cout << "Movie Booking Server started on port " << port << std::endl;
        std::cout << "Waiting for clients...\n" << std::endl;
        
        // Accept and handle clients
        while (true) {
            sockaddr_in client_addr;
            int client_addr_len = sizeof(client_addr);
            SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
            
            if (client_socket == INVALID_SOCKET) {
                std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
                continue;
            }
            
            // Handle client in a separate thread
            std::thread client_thread(&MovieBookingServer::handleClient, this, client_socket);
            client_thread.detach();
        }
        
        return true;
    }
    
    void stop() {
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
        }
    }
};

int main() {
    MovieBookingServer server(8080);
    
    std::cout << "=== Movie Ticket Booking System Server ===" << std::endl;
    std::cout << "Starting server..." << std::endl;
    
    if (server.start()) {
        std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
        
        // Keep server running
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "quit" || input == "exit") {
                break;
            }
        }
    } else {
        std::cout << "Failed to start server!" << std::endl;
    }
    
    server.stop();
    std::cout << "Server stopped." << std::endl;
    
    return 0;
}