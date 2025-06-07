// platform.h
#ifndef PLATFORM_H
#define PLATFORM_H

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    typedef SOCKET socket_t;
    typedef int socklen_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR   -1
    #define CLOSESOCKET closesocket
    #define CLEANUP_NETWORK() WSACleanup()
    #define INIT_NETWORK()                        \
        do {                                      \
            WSADATA wsa;                          \
            if (WSAStartup(MAKEWORD(2, 2), &wsa)) \
            {                                     \
                printf("WSAStartup failed.\n");   \
                exit(1);                          \
            }                                     \
        } while (0)
#else
    #include <unistd.h>
    #include <errno.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <pthread.h>
    typedef int socket_t;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR   -1
    #define CLOSESOCKET close
    #define CLEANUP_NETWORK()
    #define INIT_NETWORK()
#endif

#endif // PLATFORM_H
