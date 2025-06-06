#include <stdio.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")

SOCKET client_socket;
char message[1024];

// Receving message from server.
DWORD WINAPI receive_thread(LPVOID lpParam)
{
    char buffer[1024];
    int recv_size;

    while (1)
    {
        recv_size = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (recv_size == SOCKET_ERROR)
        {
            printf("Receive failed. Error code: %d\n", WSAGetLastError());
            break;
        }
        if (recv_size == 0)
        {
            printf("Server disconnected.\n");
            break;
        }
        buffer[recv_size] = '\0';
        printf("\nReceived: %s\n", buffer);
    }

    // Close the client socket and clean up
    closesocket(client_socket);
    WSACleanup();
    exit(0);

    return 0;
}
// sending a message.
DWORD WINAPI send_thread(LPVOID lpParam)
{
    while (1)
    {
        // Read user input for message.
        printf("You: ");
        fgets(message, sizeof(message), stdin);

        // Remove newline character from the message.
        message[strcspn(message, "\n")] = 0;

        // Check for exit command.
        if (strcmp(message, "exit") == 0)
        {
            printf("Exiting...\n");
            send(client_socket, message, strlen(message), 0); // Notify server
            closesocket(client_socket);
            WSACleanup();
            exit(0);
        }

        // Send message to server.
        if (send(client_socket, message, strlen(message), 0) < 0)
        {
            printf("Send failed. Error code: %d\n", WSAGetLastError());
        }
        else
        {
            printf("Message sent.\n");
        }
    }
    return 0;
}
int main()
{
    WSADATA wsa;
    struct sockaddr_in server_addr;

    // Initialize Winsock.
    printf("\nInitializing Winsock...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Initialized.\n");

    // Create socket.
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        printf("Could not create socket. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket created.\n");

    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Server IP address.
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080); // Server port.

    // Connect to remote server.
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        printf("Connect error. Error Code: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Connected to server.\n");

    // Create threads for sending and receiving messages.
    HANDLE recv_thread_handle = CreateThread(NULL, 0, receive_thread, NULL, 0, NULL);
    HANDLE send_thread_handle = CreateThread(NULL, 0, send_thread, NULL, 0, NULL);

    // Wait for both threads to complete.
    WaitForSingleObject(recv_thread_handle, INFINITE);
    WaitForSingleObject(send_thread_handle, INFINITE);

    // Close thread handles.
    CloseHandle(recv_thread_handle);
    CloseHandle(send_thread_handle);

    // Close the socket and cleanup.
    closesocket(client_socket);
    WSACleanup();

    return 0;
}