// server.cpp
#include "platform.h"
#include <cstdio>
#include <cstring>
using namespace std;

class AdminData
{
public:
    char Adminname[50];
    char password[50];
};

static const int mainserverPort = 12345;

int main()
{
    if (!winsock_startup())
    {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET)
    {
        perror("socket");
        winsock_cleanup();
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(mainserverPort);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR)
    {
        perror("bind");
        closesocket(serverSocket);
        winsock_cleanup();
        return 1;
    }

    if (listen(serverSocket, 8) == SOCKET_ERROR)
    {
        perror("listen");
        closesocket(serverSocket);
        winsock_cleanup();
        return 1;
    }

    cout << "Server listening on port " << mainserverPort << "...\n";

    int admin_cnt = 0, limitadmin = 2;
    while (admin_cnt < limitadmin)
    {
        SOCKET client = accept(serverSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET)
        {
            perror("accept");
            continue;
        }

        AdminData data{};
        int rec = recv(client, (char *)&data, sizeof(data), 0);
        if (rec > 0)
        {
            cout << "New User :: Username: " << data.Adminname << "  Signed in\n";
            const char *msg = "Data received by the server!";
            send(client, msg, (int)strlen(msg), 0);
            admin_cnt++;
        }
        closesocket(client);
    }

    cout << "Server cannot take more than " << limitadmin << " Admins\nShutting down.\n";
    closesocket(serverSocket);
    winsock_cleanup();
    return 0;
}
