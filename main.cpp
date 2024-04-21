#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

CRITICAL_SECTION cs;

// Fonction de traitement du client
DWORD WINAPI ClientHandler(LPVOID lpParam) {
    SOCKET clientSocket = (SOCKET)lpParam;
    char buffer[1024];
    int bytesReceived;

    // Boucle pour recevoir les messages du client
    do {
        bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            EnterCriticalSection(&cs);
            // Traitez le message du client ici selon vos besoins
            OutputDebugStringA(buffer);
            LeaveCriticalSection(&cs);
        }
    } while (bytesReceived > 0);

    // Fermer la socket client et libérer la mémoire pour la structure de données du thread
    closesocket(clientSocket);
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

    InitializeCriticalSection(&cs);

    // Initialisation Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Failed to initialize Winsock\n");
        return 1;
    }

    // Création de la socket serveur
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        printf("Failed to create server socket\n");
        WSACleanup();
        return 1;
    }

    // Définition des informations de connexion du serveur
    SOCKADDR_IN serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Écoute sur toutes les interfaces réseau
    serverAddr.sin_port = htons(12345); // Port sur lequel le serveur écoute

    // Liaison de la socket à l'adresse et au port spécifiés
    if (bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Failed to bind server socket\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    // Mise en écoute de la socket pour les connexions entrantes
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port 12345\n");

    // Accepter les connexions entrantes en boucle
    while (true) {
        // Accepter la connexion entrante
        SOCKADDR_IN clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (SOCKADDR*)&clientAddr, &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            printf("Accept failed\n");
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        // Créer un nouveau thread pour gérer ce client
        HANDLE hThread = CreateThread(NULL, 0, &ClientHandler, (LPVOID)clientSocket, 0, NULL);
        if (hThread == NULL) {
            printf("Failed to create client thread\n");
            closesocket(clientSocket);
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }

        // Libérer la ressource du thread, car il va se détacher et se terminer de manière autonome
        CloseHandle(hThread);
    }

    // Fermeture de la socket serveur et nettoyage de Winsock
    closesocket(serverSocket);
    WSACleanup();
    DeleteCriticalSection(&cs);

    return 0;
}
