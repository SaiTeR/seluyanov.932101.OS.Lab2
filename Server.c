#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>

#define PORT 1337
#define MAX_PENDING_CONNECTIONS 5
#define BUFFER_SIZE 1024

volatile sig_atomic_t isSighupReceived = 0;

void handleSighup(int signalNumber) {
    isSighupReceived = 1;
    printf("[Server / INFO]: SIGHUP signal handler activated\n");
}

void initializeServer(int *serverSocket, struct sockaddr_in *serverAddr) {
    // Создаем серверный сокет
    if ((*serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[Server / ERROR]: Socket creation error");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем адрес сервера
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr.s_addr = INADDR_ANY;
    serverAddr->sin_port = htons(PORT);

    // Привязка сокета
    if (bind(*serverSocket, (struct sockaddr*)serverAddr, sizeof(*serverAddr)) < 0) {
        perror("[Server / ERROR]: Socket bind error");
        exit(EXIT_FAILURE);
    }

    // Слушаем сокет
    if (listen(*serverSocket, MAX_PENDING_CONNECTIONS) < 0) {
        perror("[Server / ERROR]: Listen error");
        exit(EXIT_FAILURE);
    }

    printf("[Server / INFO]: Started server on port %d\n\n", PORT);
}

void setupSignalBlock(sigset_t *blockMask, sigset_t *originalMask) {
    sigemptyset(blockMask);
    sigemptyset(originalMask);
    sigaddset(blockMask, SIGHUP);
    sigprocmask(SIG_BLOCK, blockMask, originalMask);
}

void setupSignalHandler() {
    struct sigaction signalAction;
    sigaction(SIGHUP, NULL, &signalAction);
    signalAction.sa_handler = handleSighup;
    signalAction.sa_flags |= SA_RESTART;
    sigaction(SIGHUP, &signalAction, NULL);
}

void processClientConnection(int *clientSocket) {
    char readBuffer[BUFFER_SIZE] = { 0 };
    int bytesRead = read(*clientSocket, readBuffer, BUFFER_SIZE);

    if (bytesRead > 0) { 
        printf("[Server / INFO]: Message from client(%d bytes): \"%s\"\n", bytesRead, readBuffer);

        const char* serverResponse = "OH, hi, server here!";
        send(*clientSocket, serverResponse, strlen(serverResponse), 0);
    } else {
        if (bytesRead == 0) {
            printf("[Server / INFO]: Client disconnected\n\n");
        } else { 
            perror("[Server / ERROR]: Buffer reading error"); 
        }
        close(*clientSocket); 
        *clientSocket = 0; 
    } 
}

int main() {
    int serverSocket, clientSocket = 0; 
    struct sockaddr_in serverAddress; 
    fd_set activeSockets;
    sigset_t blockMask, originalMask;
    int maxSocketDescriptor;

    initializeServer(&serverSocket, &serverAddress);
    setupSignalBlock(&blockMask, &originalMask);
    setupSignalHandler();

    while (1) {
        FD_ZERO(&activeSockets); 
        FD_SET(serverSocket, &activeSockets); 
        if (clientSocket > 0) { 
            FD_SET(clientSocket, &activeSockets); 
        }
        
        maxSocketDescriptor = (clientSocket > serverSocket) ? clientSocket : serverSocket; 

        if (pselect(maxSocketDescriptor + 1, &activeSockets, NULL, NULL, NULL, &originalMask) == -1) {
            if (errno != EINTR) {
                perror("[Server / ERROR]: Pselect error"); 
                exit(EXIT_FAILURE); 
            }
            continue;
        }

        if (isSighupReceived) {
            printf("[Server / INFO]: SIGHUP received.\n");
            isSighupReceived = 0;
            continue;
        }

        // Проверяем подключения новых клиентов
        if (FD_ISSET(serverSocket, &activeSockets)) {
            if ((clientSocket = accept(serverSocket, (struct sockaddr*)&serverAddress, (socklen_t*)&serverAddress)) < 0) {
                perror("[Server / ERROR]: Accept clientSokcet error");
                continue;
            }
            printf("[Server / INFO]: New client connection established.\n");
        }

        // Обрабатываем соединение клиента
        if (clientSocket > 0 && FD_ISSET(clientSocket, &activeSockets)) { 
            processClientConnection(&clientSocket);
        }
    }

    close(serverSocket);
    return 0;
}
