#pragma once

#if defined(_WIN32)
    #define WIN32_LEAN_AND_MEAN
    #include <winsock2.h>
    #include <ws2tcpip.h>
    
    // Windows specific types
    using SocketType = SOCKET;
    using SockLen = int;
    
    // MSVC doesn't have ssize_t, define it as logic equivalent
    #include <basetsd.h>
    using ssize_t = SSIZE_T;

    #define IS_VALID_SOCKET(s) ((s) != INVALID_SOCKET)
    #define CLOSESOCKET(s) closesocket(s)
    #define LAST_ERROR WSAGetLastError()
    
#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <fcntl.h>
    #include <netdb.h>
    #include <cerrno>
    #include <cstring> // strerror

    using SocketType = int;
    using SockLen = socklen_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define IS_VALID_SOCKET(s) ((s) >= 0)
    #define CLOSESOCKET(s) close(s)
    #define LAST_ERROR errno
#endif

#include <iostream>

namespace rtype::net {

    // Initialiseur RAII pour Windows (WSAStartup)
    struct NetworkInitializer {
        NetworkInitializer() {
#if defined(_WIN32)
            WSADATA wsaData;
            int err = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (err != 0) {
                std::cerr << "WSAStartup failed with error: " << err << std::endl;
                exit(EXIT_FAILURE);
            }
#endif
        }

        ~NetworkInitializer() {
#if defined(_WIN32)
            WSACleanup();
#endif
        }
    };

    // Helper pour définir les timeouts (différent entre OS)
    inline bool SetSocketTimeout(SocketType sock, int option, long seconds, long microseconds) {
#if defined(_WIN32)
        // Windows utilise DWORD en millisecondes
        DWORD timeout = static_cast<DWORD>((seconds * 1000) + (microseconds / 1000));
        return setsockopt(sock, SOL_SOCKET, option, (const char*)&timeout, sizeof(timeout)) != SOCKET_ERROR;
#else
        // Linux utilise struct timeval
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = microseconds;
        return setsockopt(sock, SOL_SOCKET, option, &tv, sizeof(tv)) == 0;
#endif
    }
}
