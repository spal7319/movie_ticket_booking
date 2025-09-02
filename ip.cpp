#include <iostream>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h> // For gethostname

int main() {
    char host[256];
    if (gethostname(host, sizeof(host)) != 0) {
        std::cerr << "Error getting hostname." << std::endl;
        return EXIT_FAILURE;
    }

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Force IPv4
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, NULL, &hints, &res) != 0) {
        std::cerr << "Unable to get address info." << std::endl;
        return EXIT_FAILURE;
    }

    char ip[INET_ADDRSTRLEN];
    bool found = false;

    for (p = res; p != nullptr; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = reinterpret_cast<struct sockaddr_in*>(p->ai_addr);
            void *addr = &(ipv4->sin_addr);

            if (inet_ntop(AF_INET, addr, ip, sizeof(ip)) == nullptr) {
                std::cerr << "Error converting IP address." << std::endl;
                continue;
            }

            found = true;
            break; // Take the first valid IPv4 address
        }
    }

    freeaddrinfo(res); // Free the linked list

    if (found) {
        std::cout << "Your IPv4 address is: " << ip << std::endl;
    } else {
        std::cerr << "No IPv4 address found." << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
