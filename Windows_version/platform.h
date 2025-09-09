// platform.h
#pragma once
// Windows compatibility layer for sockets + shared memory
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "Ws2_32.lib")
#else
  #error "This port is intended for Windows only."
#endif

#include <iostream>
#include <string>

struct Person {
    char id[50];
    int curr_bal;
    int total_spend;
};

// Shared memory helpers (File Mapping)
inline HANDLE create_or_open_person_mapping(std::size_t count = 3) {
    return CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                              (DWORD)(count * sizeof(Person)),
                              L"Local\\MOVIE_PERSON_SHM");
}

inline Person* map_person_view(HANDLE hMap) {
    void* p = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    return reinterpret_cast<Person*>(p);
}

inline void unmap_person_view(Person* p) {
    if (p) UnmapViewOfFile(p);
}

inline void close_mapping(HANDLE hMap) {
    if (hMap) CloseHandle(hMap);
}

// Socket helpers
inline bool winsock_startup() {
    WSADATA wsa; 
    return WSAStartup(MAKEWORD(2,2), &wsa) == 0;
}

inline void winsock_cleanup() {
    WSACleanup();
}
