// tcp_client.c
#define _WIN32_WINNT 0x501

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib,"ws2_32.lib") // Link with Winsock library

#define PORT "65432"
#define BUFFER_SIZE 1024

volatile int flag = 0;
SOCKET sockfd = INVALID_SOCKET;
char screen_name[50];

void str_overwrite_stdout() {
    printf("\rYou: ");
    fflush(stdout);
}

void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

DWORD WINAPI receive_handler(LPVOID arg) {
    char message[BUFFER_SIZE];
    int receive;
    while (1) {
        receive = recv(sockfd, message, BUFFER_SIZE, 0);
        if (receive > 0) {
            printf("\r%s", message);
            printf("\n");
            str_overwrite_stdout();
        } else if (receive == 0) {
            printf("Server disconnected.\n");
            break;
        } else {
            // Error
            break;
        }
        memset(message, 0, sizeof(message));
    }
    flag = 1;
    return 0;
}

DWORD WINAPI send_handler(LPVOID arg) {
    char message[BUFFER_SIZE];
    while (1) {
        str_overwrite_stdout();
        fgets(message, BUFFER_SIZE, stdin);
        str_trim_lf(message, BUFFER_SIZE);

        if (strcmp(message, "") == 0) {
            continue;
        }

        if (send(sockfd, message, strlen(message), 0) == SOCKET_ERROR) {
            printf("Error sending message\n");
            break;
        }

        if (strcmp(message, "exit") == 0) {
            break;
        }

        memset(message, 0, sizeof(message));
    }
    flag = 1;
    return 0;
}

int main() {
    WSADATA wsaData;
    int result;

    // Initialize Winsock
    if ((result = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        printf("WSAStartup failed: %d\n", result);
        return 1;
    }

    printf("Enter server IP address: ");
    char ip[100];
    fgets(ip, 100, stdin);
    str_trim_lf(ip, strlen(ip));

    struct addrinfo hints, *server_info, *p;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM;

    if ((result = getaddrinfo(ip, PORT, &hints, &server_info)) != 0) {
        printf("getaddrinfo failed: %d\n", result);
        WSACleanup();
        return 1;
    }

    // Create socket and connect
    for (p = server_info; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == INVALID_SOCKET) {
            continue;
        }

        if (connect(sockfd, p->ai_addr, (int)p->ai_addrlen) == SOCKET_ERROR) {
            closesocket(sockfd);
            sockfd = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(server_info);

    if (sockfd == INVALID_SOCKET) {
        printf("Unable to connect to server\n");
        WSACleanup();
        return 1;
    }

    // Set screen name
    printf("Enter your screen name: ");
    fgets(screen_name, 50, stdin);
    str_trim_lf(screen_name, strlen(screen_name));

    if (send(sockfd, screen_name, 50, 0) == SOCKET_ERROR) {
        printf("Error sending screen name\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    printf("=== Welcome to the chatroom ===\n");

    HANDLE hSendThread = CreateThread(NULL, 0, send_handler, NULL, 0, NULL);
    HANDLE hReceiveThread = CreateThread(NULL, 0, receive_handler, NULL, 0, NULL);

    if (hSendThread == NULL || hReceiveThread == NULL) {
        printf("Could not create threads\n");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // Wait for threads to finish
    WaitForSingleObject(hSendThread, INFINITE);
    WaitForSingleObject(hReceiveThread, INFINITE);

    CloseHandle(hSendThread);
    CloseHandle(hReceiveThread);

    closesocket(sockfd);
    WSACleanup();

    return 0;
}
