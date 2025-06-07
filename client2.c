#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "platform.h"

#define PORT 8080

socket_t client_socket;
char message[1024];

// Receve message from server.
#ifdef _WIN32
DWORD WINAPI receve_thread(LPVOID lpParam)
#else
void *receve_thread(void *lpParam)
#endif
{
    char buffer[1024];
    int recv_size;

    while (1)
    {
        recv_size = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (recv_size == SOCKET_ERROR)
        {
#ifdef _WIN32
            printf("Receive failed. Error code: %d\n", WSAGetLastError());
#else
            perror("Receive failed");
#endif
            break;
        }
        
        if (recv_size == 0)
        {
            printf("server disconnected.\n");
            break;
        }
            
        buffer[recv_size] = '\0';
        printf("\nReceved: %s\n", buffer);
    }

    CLOSESOCKET(client_socket);
    CLEANUP_NETWORK();
    
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif

}
// Sending message to server.
#ifdef _WIN32
DWORD WINAPI send_thread(LPVOID lpParam)
#else
void *send_thread(LPVOID lpParam)
#endif
{
    while (1)
    {
        printf("YOU: ");
        fgets(message, sizeof(message), stdin);
        message[strcspn(message, "\n")] = 0;
        if (strcmp(message, "exit") == 0)
        {
            printf("\nExiting....\n");
            send(client_socket, message, strlen(message), 0);
            CLOSESOCKET(client_socket);
            CLEANUP_NETWORK();
            exit(0);
        }
        if (send(client_socket, message, strlen(message), 0) < 0)
        {
#ifdef _WIN32
            printf("Send failed. Error code: %d\n", WSAGetLastError());
#else
            perror("Send failed");
#endif
        }
        else
        {
            printf("Message sent.\n");
        }
    }
#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif

}
int main(){
    INIT_NETWORK();

    struct sockaddr_in server_addr;

    if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
#ifdef _WIN32
        printf("Could not create socket. Error Code: %d\n", WSAGetLastError());
#else
        perror("Could not create socket");
#endif
        return 1;
    }
    printf("\nSocket created.\n");

    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if(connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
    {
#ifdef _WIN32
        printf("Connect error. Error Code: %d\n", WSAGetLastError());
#else
        perror("Connect error");
#endif
        return 1;
    }
    printf("connected to server.\n");

#ifdef _WIN32
    HANDLE receve_thread_handle = CreateThread(NULL, 0, receve_thread, NULL, 0, NULL);
    HANDLE send_thread_handle = CreateThread(NULL, 0, send_thread, NULL, 0, NULL);

    WaitForSingleObject(receve_thread_handle, INFINITE);
    WaitForSingleObject(send_thread_handle, INFINITE);

    CloseHandle(receve_thread_handle);
    CloseHandle(send_thread_handle);
#else
    pthread_t recv_thread_id , send_thread_id;
    pthread_create(&receve_thread_id, NULL, receve_thread, NULL);
    pthread_create(&send_thread_id, NULL, send_thread, NULL);
    
    pthread_join(recv_thread_id, NULL);
    pthread_join(send_thread_id, NULL);
#endif
    
    CLEANUP_NETWORK();
    return 0;
}

// For windows
// gcc client.c -o client.exe -lws2_32
// ./client.exe

// For linux
// gcc client.c -o client -lpthread
// ./client
