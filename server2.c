#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>

#define MAX_CLIENT 10
#pragma comment(lib, "ws2_32.lib") // LINK WITH WINSOCK LIBRARY.

CRITICAL_SECTION cs; // It is used for synchronization of a variabe or any resource to avoide race condition or crash.

int client_count = 0;

struct client_info
{
    SOCKET socket;
    char client_name[50];
};
struct client_info clients[MAX_CLIENT];

// Broadcast a message to all clients except the sender
void broadcast_message(SOCKET sender_socket, const char *message)
{
    EnterCriticalSection(&cs);
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].socket != sender_socket)
        { // Skip the sender
            // Send the message to the client
            if (send(clients[i].socket, message, strlen(message), 0) == SOCKET_ERROR)
            {
                printf("Send failed. Error code: %d\n", WSAGetLastError());
            }
        }
    }
    LeaveCriticalSection(&cs);
}

DWORD WINAPI client_thread(LPVOID lpParam)
{
    SOCKET client_socket = (SOCKET)lpParam;
    char buffer[1024];
    int recv_size;

    // Generate a unique client name.
    char client_name[50];
    EnterCriticalSection(&cs);
    if (client_count >= MAX_CLIENT)
    {
        printf("Maximum clients reached. Connection refused.\n");
        LeaveCriticalSection(&cs);
        closesocket(client_socket);
        return 0;
    }

    
    sprintf(client_name, "client%d", client_count + 1);
    clients[client_count].socket = client_socket;
    strcpy(clients[client_count].client_name, client_name);
    client_count++;
    printf("%s connected.\n", client_name);
    LeaveCriticalSection(&cs);

    // Step 7: Receive and echo the data.
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
            printf("%s disconnected.\n", client_name);
            break;
        }

        buffer[recv_size] = '\0';

        // Check for exit command.
        if (strcmp(buffer, "exit") == 0)
        {
            printf("%s disconnected.\n", client_name);
            break;
        }

        broadcast_message(client_socket, buffer);
    }

    // Remove the client from the global client array.
    EnterCriticalSection(&cs); // it is a gate way to enter this section only one client or process at a time
    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].socket == client_socket)
        {
            for (int j = i; j < client_count - 1; j++)
            {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    LeaveCriticalSection(&cs); // it is a gate way to exit this section only one client or process at a time

    // Close the client socket.
    closesocket(client_socket);

    return 0;
}

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE hprivinst, LPSTR args, int ncmdshow)
{
    InitializeCriticalSection(&cs);
    WSADATA wsa;
    SOCKET client_socket, server_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len, recv_size;
    char buffer[1024];

    // STEP 1: INITIALIZE WINSOCK.
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        MessageBox(NULL, "Winsock not initialized", "ERROR", MB_OK);
        printf("Failed to initialize wisock. Error code %d", WSAGetLastError());
        return -1;
    }
    printf("Winsock Initialized.\n");

    // STEP 2: CREATE A SOCKET.
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET)
    {
        MessageBox(NULL, "Error in socket creation", "ERROR", MB_OK);
        printf("Failed to create socket.Error code %d\n", WSAGetLastError());
        WSACleanup();
        return -1;
    }
    printf("Socket created.\n");

    // step 3: PREPAIR THE serveradde_in structure.

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // STEP 4: BIND THE SOCKET.

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        printf("BIND FAILED . ERROR CODE %d\n", WSAGetLastError());
        MessageBox(NULL, "Bind failed", "BIND ERROR", 0);
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // STEP 5: LISTEN FOR INCOMMING CONECTION.
    listen(server_socket, MAX_CLIENT);
    printf("Wating for incomming connections....\n");

    // STEP 6: ACCEPT AN INCOMING CONNECTION.

    while (1)
    {
        client_addr_len = sizeof(struct sockaddr_in);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET)
        {

            printf("Accept failed . Error code: %d\n", WSAGetLastError());
            MessageBox(NULL, "Accept failed", "ERROR", 0);
            closesocket(server_socket);
            WSACleanup();
            return 1;
        }
        printf("Connection accepted.\n");
        // Create a new thread to handel the client.

        HANDLE hThread = CreateThread(
            NULL,
            0,
            client_thread,
            (LPVOID)client_socket,
            0,
            NULL);
        if (hThread == NULL)
        {
            printf("Error creating thread. Error code: %d\n", GetLastError());
            closesocket(client_socket); // Clean up if thread creation fails
        }
        else
        {
            CloseHandle(hThread); // We don't need to wait for this thread to finish
        }
    }
    closesocket(server_socket);
    WSACleanup();
    DeleteCriticalSection(&cs);
    return 0;
}