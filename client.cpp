#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "structures.h"

#pragma comment(lib, "ws2_32.lib")

class MovieBookingClient {
private:
    SOCKET client_socket;
    std::string server_ip;
    int server_port;
    int user_id;
    UserType user_type;
    bool is_authenticated;
    
    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }
    
    std::string sendRequest(const std::string& request) {
        send(client_socket, request.c_str(), request.length(), 0);
        
        char buffer[4096];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received > 0) {
            return std::string(buffer);
        }
        return "";
    }
    
    void displayMovies() {
        std::string response = sendRequest("VIEW_MOVIES");
        auto parts = split(response, '|');
        
        if (parts.size() >= 2 && parts[0] == "MOVIES") {
            std::cout << "\n=== Available Movies ===\n";
            std::cout << "ID\tMovie Name\t\tDuration\tPrice\n";
            std::cout << "---------------------------------------------------\n";
            
            auto movies = split(parts[1], ';');
            for (const auto& movie_str : movies) {
                if (!movie_str.empty()) {
                    auto movie_parts = split(movie_str, ':');
                    if (movie_parts.size() >= 4) {
                        std::cout << movie_parts[0] << "\t" << movie_parts[1] << "\t\t" 
                                 << movie_parts[2] << " min\t$" << movie_parts[3] << "\n";
                    }
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "No movies available.\n";
        }
    }
    
    void displayShows(int movie_id) {
        std::string request = "VIEW_SHOWS " + std::to_string(movie_id);
        std::string response = sendRequest(request);
        auto parts = split(response, '|');
        
        if (parts.size() >= 2 && parts[0] == "SHOWS") {
            std::cout << "\n=== Available Shows ===\n";
            std::cout << "Show ID\tDate\t\tTime\tAvailable Seats\n";
            std::cout << "-------------------------------------------\n";
            
            auto shows = split(parts[1], ';');
            for (const auto& show_str : shows) {
                if (!show_str.empty()) {
                    auto show_parts = split(show_str, ':');
                    if (show_parts.size() >= 4) {
                        std::cout << show_parts[0] << "\t" << show_parts[1] << "\t" 
                                 << show_parts[2] << "\t" << show_parts[3] << "\n";
                    }
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "No shows available for this movie.\n";
        }
    }
    
    void displaySeats(int show_id) {
        std::string request = "VIEW_SEATS " + std::to_string(show_id);
        std::string response = sendRequest(request);
        auto parts = split(response, '|');
        
        if (parts.size() >= 2 && parts[0] == "SEATS") {
            std::cout << "\n=== Seat Availability ===\n";
            std::cout << "Seat Number\tStatus\n";
            std::cout << "-------------------\n";
            
            auto seats = split(parts[1], ';');
            for (const auto& seat_str : seats) {
                if (!seat_str.empty()) {
                    auto seat_parts = split(seat_str, ':');
                    if (seat_parts.size() >= 2) {
                        std::cout << seat_parts[0] << "\t\t" << seat_parts[1] << "\n";
                    }
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "No seat information available.\n";
        }
    }
    
    void bookTickets() {
        displayMovies();
        
        int movie_id;
        std::cout << "Enter Movie ID: ";
        std::cin >> movie_id;
        
        displayShows(movie_id);
        
        int show_id;
        std::cout << "Enter Show ID: ";
        std::cin >> show_id;
        
        displaySeats(show_id);
        
        int num_seats;
        std::cout << "Enter number of seats to book: ";
        std::cin >> num_seats;
        
        std::vector<int> seat_numbers;
        std::cout << "Enter seat numbers: ";
        for (int i = 0; i < num_seats; i++) {
            int seat_num;
            std::cin >> seat_num;
            seat_numbers.push_back(seat_num);
        }
        
        std::string request = "BOOK_TICKETS " + std::to_string(show_id) + " " + std::to_string(num_seats);
        for (int seat_num : seat_numbers) {
            request += " " + std::to_string(seat_num);
        }
        
        std::string response = sendRequest(request);
        auto parts = split(response, '|');
        
        if (parts.size() >= 3 && parts[0] == "BOOKING_SUCCESS") {
            std::cout << "\n=== Booking Successful! ===\n";
            std::cout << "Booking ID: " << parts[1] << "\n";
            std::cout << "Total Amount: $" << parts[2] << "\n";
            std::cout << "Thank you for booking with us!\n\n";
        } else {
            std::cout << "Booking failed! Seats may already be booked.\n\n";
        }
    }
    
    void viewMyBookings() {
        std::string response = sendRequest("MY_BOOKINGS");
        auto parts = split(response, '|');
        
        if (parts.size() >= 2 && parts[0] == "MY_BOOKINGS") {
            std::cout << "\n=== My Bookings ===\n";
            std::cout << "Booking ID\tMovie\t\tDate\t\tTime\tSeats\tAmount\n";
            std::cout << "---------------------------------------------------------------\n";
            
            auto bookings = split(parts[1], ';');
            for (const auto& booking_str : bookings) {
                if (!booking_str.empty()) {
                    auto booking_parts = split(booking_str, ':');
                    if (booking_parts.size() >= 6) {
                        std::cout << booking_parts[0] << "\t\t" << booking_parts[1] << "\t" 
                                 << booking_parts[2] << "\t" << booking_parts[3] << "\t" 
                                 << booking_parts[4] << "\t$" << booking_parts[5] << "\n";
                    }
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "No bookings found.\n";
        }
    }
    
    void clientMenu() {
        int choice;
        
        while (true) {
            std::cout << "\n=== Client Menu ===\n";
            std::cout << "1. View Movies\n";
            std::cout << "2. Book Tickets\n";
            std::cout << "3. View My Bookings\n";
            std::cout << "4. Logout\n";
            std::cout << "Enter choice: ";
            std::cin >> choice;
            
            switch (choice) {
                case 1:
                    displayMovies();
                    break;
                case 2:
                    bookTickets();
                    break;
                case 3:
                    viewMyBookings();
                    break;
                case 4:
                    return;
                default:
                    std::cout << "Invalid choice!\n";
            }
        }
    }
    
    void adminMenu() {
        int choice;
        
        while (true) {
            std::cout << "\n=== Admin Menu ===\n";
            std::cout << "1. View Movies\n";
            std::cout << "2. Add Movie\n";
            std::cout << "3. Add Show\n";
            std::cout << "4. View All Bookings\n";
            std::cout << "5. Logout\n";
            std::cout << "Enter choice: ";
            std::cin >> choice;
            
            switch (choice) {
                case 1:
                    displayMovies();
                    break;
                case 2:
                    addMovie();
                    break;
                case 3:
                    addShow();
                    break;
                case 4:
                    viewAllBookings();
                    break;
                case 5:
                    return;
                default:
                    std::cout << "Invalid choice!\n";
            }
        }
    }
    
    void addMovie() {
        std::cin.ignore(); // Clear input buffer
        std::string movie_name;
        int duration;
        double price;
        
        std::cout << "Enter movie name (use _ for spaces): ";
        std::getline(std::cin, movie_name);
        
        // Replace spaces with underscores for transmission
        for (auto& c : movie_name) {
            if (c == ' ') c = '_';
        }
        
        std::cout << "Enter duration (minutes): ";
        std::cin >> duration;
        
        std::cout << "Enter ticket price: $";
        std::cin >> price;
        
        std::string request = "ADD_MOVIE " + movie_name + " " + std::to_string(duration) + " " + std::to_string(price);
        std::string response = sendRequest(request);
        
        if (response == "MOVIE_ADDED_SUCCESS") {
            std::cout << "Movie added successfully!\n";
        } else {
            std::cout << "Failed to add movie!\n";
        }
    }
    
    void addShow() {
        displayMovies();
        
        int movie_id, total_seats;
        std::string date, time;
        
        std::cout << "Enter Movie ID: ";
        std::cin >> movie_id;
        
        std::cout << "Enter show date (YYYY-MM-DD): ";
        std::cin >> date;
        
        std::cout << "Enter show time (HH:MM): ";
        std::cin >> time;
        
        std::cout << "Enter total seats: ";
        std::cin >> total_seats;
        
        std::string request = "ADD_SHOW " + std::to_string(movie_id) + " " + date + " " + time + " " + std::to_string(total_seats);
        std::string response = sendRequest(request);
        
        if (response == "SHOW_ADDED_SUCCESS") {
            std::cout << "Show added successfully!\n";
        } else {
            std::cout << "Failed to add show!\n";
        }
    }
    
    void viewAllBookings() {
        std::string response = sendRequest("ALL_BOOKINGS");
        auto parts = split(response, '|');
        
        if (parts.size() >= 2 && parts[0] == "ALL_BOOKINGS") {
            std::cout << "\n=== All Bookings ===\n";
            std::cout << "Booking ID\tUser ID\tMovie\t\tDate\t\tTime\tSeats\tAmount\n";
            std::cout << "-----------------------------------------------------------------------\n";
            
            auto bookings = split(parts[1], ';');
            for (const auto& booking_str : bookings) {
                if (!booking_str.empty()) {
                    auto booking_parts = split(booking_str, ':');
                    if (booking_parts.size() >= 7) {
                        std::cout << booking_parts[0] << "\t\t" << booking_parts[1] << "\t" 
                                 << booking_parts[2] << "\t" << booking_parts[3] << "\t" 
                                 << booking_parts[4] << "\t" << booking_parts[5] << "\t$" 
                                 << booking_parts[6] << "\n";
                    }
                }
            }
            std::cout << std::endl;
        } else {
            std::cout << "No bookings found.\n";
        }
    }
    
public:
    MovieBookingClient(const std::string& ip = "127.0.0.1", int port = 8080) 
        : server_ip(ip), server_port(port), user_id(0), user_type(CLIENT), is_authenticated(false) {
    }
    
    ~MovieBookingClient() {
        disconnect();
        WSACleanup();
    }
    
    bool connect() {
        // Initialize Winsock
        WSADATA wsaData;
        int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (result != 0) {
            std::cerr << "WSAStartup failed: " << result << std::endl;
            return false;
        }
        
        // Create socket
        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return false;
        }
        
        // Setup server address
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);
        
        // Connect to server
        if (::connect(client_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
            closesocket(client_socket);
            WSACleanup();
            return false;
        }
        
        std::cout << "Connected to server at " << server_ip << ":" << server_port << std::endl;
        return true;
    }
    
    void disconnect() {
        if (client_socket != INVALID_SOCKET) {
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
        }
    }
    
    bool login() {
        std::string username, password;
        
        std::cout << "\n=== Login ===\n";
        std::cout << "Username: ";
        std::cin >> username;
        std::cout << "Password: ";
        std::cin >> password;
        
        std::string request = "LOGIN " + username + " " + password;
        std::string response = sendRequest(request);
        
        auto parts = split(response, '|');
        if (parts.size() >= 3 && parts[0] == "LOGIN_SUCCESS") {
            user_id = std::stoi(parts[1]);
            user_type = (parts[2] == "ADMIN") ? ADMIN : CLIENT;
            is_authenticated = true;
            
            std::cout << "Login successful! Welcome " << username << " (" 
                     << (user_type == ADMIN ? "Admin" : "Client") << ")\n";
            return true;
        } else {
            std::cout << "Login failed! Invalid credentials.\n";
            return false;
        }
    }
    
    void run() {
        if (!connect()) {
            std::cout << "Failed to connect to server!\n";
            return;
        }
        
        std::cout << "\n=== Movie Ticket Booking System ===\n";
        std::cout << "Default users: admin/admin123 (Admin), user/user123 (Client)\n";
        
        if (login()) {
            if (user_type == ADMIN) {
                adminMenu();
            } else {
                clientMenu();
            }
        }
        
        disconnect();
    }
};

int main() {
    MovieBookingClient client;
    client.run();
    
    return 0;
}