// tcp_server.c
#define _WIN32_WINNT 0x501

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib") // Link with Winsock library

#define PORT "65432"
#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

typedef struct {
    SOCKET sockfd;
    char screen_name[50];
} client_t;

client_t *clients[MAX_CLIENTS];
CRITICAL_SECTION clients_cs;

void broadcast_message(char *message, SOCKET sender_sockfd) {
    EnterCriticalSection(&clients_cs);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->sockfd != sender_sockfd) {
                if (send(clients[i]->sockfd, message, strlen(message), 0) == SOCKET_ERROR) {
                    printf("Error sending message to %s\n", clients[i]->screen_name);
                    continue;
                }
            }
        }
    }

    LeaveCriticalSection(&clients_cs);
}

DWORD WINAPI handle_client(void *arg) {
    char buffer[BUFFER_SIZE];
    char message[BUFFER_SIZE + 50];
    int leave_flag = 0;

    client_t *cli = (client_t *)arg;

    // Receive screen name
    int recv_size = recv(cli->sockfd, cli->screen_name, 50, 0);
    if (recv_size <= 0 || strlen(cli->screen_name) < 2 || strlen(cli->screen_name) >= 50 - 1) {
        printf("Didn't enter the name correctly.\n");
        leave_flag = 1;
    } else {
        sprintf(buffer, "%s has joined the chat.\n", cli->screen_name);
        printf("%s", buffer);
        broadcast_message(buffer, cli->sockfd);
    }

    memset(buffer, 0, BUFFER_SIZE);

    while (1) {
        if (leave_flag) {
            break;
        }

        int receive = recv(cli->sockfd, buffer, BUFFER_SIZE, 0);
        if (receive > 0) {
            if (strlen(buffer) > 0) {
                sprintf(message, "%s: %s\n", cli->screen_name, buffer);
                printf("%s", message);
                broadcast_message(message, cli->sockfd);
            }
        } else if (receive == 0 || strcmp(buffer, "exit") == 0) {
            sprintf(buffer, "%s has left the chat.\n", cli->screen_name);
            printf("%s", buffer);
            broadcast_message(buffer, cli->sockfd);
            leave_flag = 1;
        } else {
            printf("Error receiving message from %s\n", cli->screen_name);
            leave_flag = 1;
        }

        memset(buffer, 0, BUFFER_SIZE);
        memset(message, 0, BUFFER_SIZE + 50);
    }

    // Remove client
    closesocket(cli->sockfd);
    EnterCriticalSection(&clients_cs);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            if (clients[i]->sockfd == cli->sockfd) {
                clients[i] = NULL;
                break;
            }
        }
    }

    LeaveCriticalSection(&clients_cs);
    free(cli);
    return 0;
}

DWORD WINAPI server_send_messages(void *arg) {
    char message[BUFFER_SIZE + 50];

    while (1) {
        fgets(message, BUFFER_SIZE, stdin);
        if (strcmp(message, "exit\n") == 0) {
            break;
        }
        char server_message[BUFFER_SIZE + 50];
        sprintf(server_message, "server: %s", message);
        broadcast_message(server_message, INVALID_SOCKET);
    }

    return 0;
}

int main() {
    WSADATA wsaData;
    SOCKET listen_sock, new_sock;
    struct addrinfo hints, *server_info, *p;
    int result;
    HANDLE hThread;

    // Initialize Winsock
    if ((result = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    InitializeCriticalSection(&clients_cs);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((result = getaddrinfo(NULL, PORT, &hints, &server_info)) != 0) {
        printf("getaddrinfo failed: %d\n", result);
        WSACleanup();
        return 1;
    }

    // Create and bind socket
    for (p = server_info; p != NULL; p = p->ai_next) {
        listen_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listen_sock == INVALID_SOCKET) {
            continue;
        }

        if (bind(listen_sock, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR) {
            closesocket(listen_sock);
            continue;
        }

        break;
    }

    freeaddrinfo(server_info);

    if (listen_sock == INVALID_SOCKET) {
        printf("Could not create or bind socket\n");
        WSACleanup();
        return 1;
    }

    // Listen
    if (listen(listen_sock, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    printf("=== Server is listening on port %s ===\n", PORT);

    // Create thread for server operator to send messages
    HANDLE hSendThread = CreateThread(NULL, 0, server_send_messages, NULL, 0, NULL);
    if (hSendThread == NULL) {
        printf("Could not create send thread\n");
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    while (1) {
        new_sock = accept(listen_sock, NULL, NULL);

        if (new_sock == INVALID_SOCKET) {
            printf("Accept failed\n");
            continue;
        }

        // Check if max clients is reached
        EnterCriticalSection(&clients_cs);
        int clients_count = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i])
                clients_count++;
        }
        LeaveCriticalSection(&clients_cs);

        if (clients_count >= MAX_CLIENTS) {
            printf("Max clients reached. Connection rejected.\n");
            closesocket(new_sock);
            continue;
        }

        // Client settings
        client_t *cli = (client_t *)malloc(sizeof(client_t));
        cli->sockfd = new_sock;
        strcpy(cli->screen_name, "Anonymous");

        // Add client to the clients array
        EnterCriticalSection(&clients_cs);
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (!clients[i]) {
                clients[i] = cli;
                break;
            }
        }
        LeaveCriticalSection(&clients_cs);

        // Create thread
        hThread = CreateThread(NULL, 0, handle_client, (void *)cli, 0, NULL);
        if (hThread == NULL) {
            printf("Could not create thread for new client\n");
            free(cli);
            closesocket(new_sock);
            continue;
        }
        CloseHandle(hThread);
    }

    // Cleanup
    DeleteCriticalSection(&clients_cs);
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}
