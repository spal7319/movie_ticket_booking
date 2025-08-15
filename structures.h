#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>

enum UserType { ADMIN = 1, CLIENT = 2 };
enum BookingStatus { CONFIRMED = 1, CANCELLED = 2 };
enum SeatStatus { AVAILABLE = 1, BOOKED = 2 };
enum MovieStatus { ACTIVE = 1, INACTIVE = 2 };

struct User {
    int user_id;
    std::string username;
    std::string password;
    UserType user_type;
    
    User() : user_id(0), user_type(CLIENT) {}
    User(int id, std::string uname, std::string pass, UserType type) 
        : user_id(id), username(uname), password(pass), user_type(type) {}
};

struct Movie {
    int movie_id;
    std::string movie_name;
    int duration;
    double price;
    MovieStatus status;
    
    Movie() : movie_id(0), duration(0), price(0.0), status(ACTIVE) {}
    Movie(int id, std::string name, int dur, double pr, MovieStatus st = ACTIVE) 
        : movie_id(id), movie_name(name), duration(dur), price(pr), status(st) {}
};

struct Show {
    int show_id;
    int movie_id;
    std::string show_date;
    std::string show_time;
    int total_seats;
    int available_seats;
    
    Show() : show_id(0), movie_id(0), total_seats(100), available_seats(100) {}
    Show(int sid, int mid, std::string date, std::string time, int total = 100) 
        : show_id(sid), movie_id(mid), show_date(date), show_time(time), 
          total_seats(total), available_seats(total) {}
};

struct Booking {
    int booking_id;
    int user_id;
    int show_id;
    int seats_booked;
    double total_amount;
    std::string booking_time;
    BookingStatus status;
    
    Booking() : booking_id(0), user_id(0), show_id(0), seats_booked(0), 
                total_amount(0.0), status(CONFIRMED) {}
};

struct Seat {
    int show_id;
    int seat_number;
    SeatStatus status;
    int booked_by_user_id;
    
    Seat() : show_id(0), seat_number(0), status(AVAILABLE), booked_by_user_id(0) {}
    Seat(int sid, int snum) : show_id(sid), seat_number(snum), 
                               status(AVAILABLE), booked_by_user_id(0) {}
};

// Message structure for client-server communication
struct Message {
    std::string command;
    std::string data;
    int user_id;
    UserType user_type;
    
    Message() : user_id(0), user_type(CLIENT) {}
    Message(std::string cmd, std::string d = "", int uid = 0, UserType ut = CLIENT) 
        : command(cmd), data(d), user_id(uid), user_type(ut) {}
};

#endif