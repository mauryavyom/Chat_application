// server.c
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "platform.h"

#define MAX_CLIENT 10
#define PORT 8080

#ifdef _WIN32
CRITICAL_SECTION cs; // for sync in windows
#else
#include <pthread.h>
pthread_mutex_t cs = PTHREAD_MUTEX_INITIALIZER;
#endif

int client_count = 0;

struct client_info {
    socket_t socket;
    char client_name[50];
};

struct client_info clients[MAX_CLIENT];

// Enter and leave critical section wrappers
void enter_critical_section() {
#ifdef _WIN32
    EnterCriticalSection(&cs);
#else
    pthread_mutex_lock(&cs);
#endif
}

void leave_critical_section() {
#ifdef _WIN32
    LeaveCriticalSection(&cs);
#else
    pthread_mutex_unlock(&cs);
#endif
}

// Brodcast message to all clients except sender.
void broadcast_message(socket_t sender_socket, const char *message) {
    enter_critical_section();
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != sender_socket) {
            if (send(clients[i].socket, message, (int)strlen(message), 0) == SOCKET_ERROR) {
                printf("Send failed. Error code: %d\n",
#ifdef _WIN32
                       WSAGetLastError()
#else
                       errno
#endif
                );
            }
        }
    }
    leave_critical_section();
}

#ifdef _WIN32
DWORD WINAPI client_thread(LPVOID lpParam)
#else
void *client_thread(void *lpParam)
#endif
{
    socket_t client_socket = (socket_t)(uintptr_t)(lpParam);
    char buffer[1024];
    int recv_size;
    char client_name[50];

    enter_critical_section();
    if (client_count >= MAX_CLIENT) {
        printf("Max clients reached. Connection refused.\n");
        leave_critical_section();
        CLOSESOCKET(client_socket);
#ifdef _WIN32
        return 0;
#else
        return NULL;
#endif
    }

    sprintf(client_name, "client%d", client_count + 1);
    clients[client_count].socket = client_socket;
    strcpy(clients[client_count].client_name, client_name);
    client_count++;
    printf("%s connected.\n", client_name);
    leave_critical_section();

    while (1) {
        recv_size = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (recv_size <= 0) {
            printf("%s disconnected.\n", client_name);
            break;
        }
        buffer[recv_size] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            printf("%s sent exit command.\n", client_name);
            break;
        }

        broadcast_message(client_socket, buffer);
    }

    // REmove clients
    enter_critical_section();
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == client_socket) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    leave_critical_section();

    CLOSESOCKET(client_socket);
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

int main() {
    INIT_NETWORK();
#ifdef _WIN32
    InitializeCriticalSection(&cs);
#endif

    socket_t server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("Socket creation failed. Error: %d\n",
#ifdef _WIN32
               WSAGetLastError()
#else
               errno
#endif
        );
        CLEANUP_NETWORK();
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed. Error: %d\n",
#ifdef _WIN32
               WSAGetLastError()
#else
               errno
#endif
        );
        CLOSESOCKET(server_socket);
        CLEANUP_NETWORK();
        return 1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        printf("Listen failed. Error: %d\n",
#ifdef _WIN32
               WSAGetLastError()
#else
               errno
#endif
        );
        CLOSESOCKET(server_socket);
        CLEANUP_NETWORK();
        return 1;
    }

    printf("Server started on port %d. Waiting for clients...\n", PORT);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) {
            printf("Accept failed. Error: %d\n",
#ifdef _WIN32
                   WSAGetLastError()
#else
                   errno
#endif
            );
            continue;
        }

#ifdef _WIN32
        CreateThread(NULL, 0, client_thread, (LPVOID)(uintptr_t)client_socket, 0, NULL);
#else
        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, (void *)(uintptr_t)client_socket);
        pthread_detach(tid);
#endif
    }

    CLOSESOCKET(server_socket);
#ifdef _WIN32
    DeleteCriticalSection(&cs);
#endif
    CLEANUP_NETWORK();
    return 0;
}

// For windows
// gcc server.c -o server.exe -lws2_32
// ./server.exe

// For linux
// gcc server.c -o server -lpthread
// ./server