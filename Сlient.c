#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 1337
#define BUFFER_SIZE 1024

int main() {
    struct sockaddr_in serverAddress;
    int clientSocket = 0;
    char* clientMessage = "Hi server, its me - client";
    char responseBuffer[BUFFER_SIZE] = {0};

    // Создаем сокет для клиента
    if ((clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[Client / ERROR]: Socket creation error\n");
        exit(EXIT_FAILURE);
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    // Конвертируем адрес сервера в binary
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr) <= 0) {
        printf("[Client / ERROR]: Address not supported\n");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем соединение клиент-сервер
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("[Client / ERROR]: Connection failed\n");
        exit(EXIT_FAILURE);
    } else {
        printf("[Client / INFO]: Connection successful\n");
    }

    // Отправляем сообщение на сервер
    send(clientSocket, clientMessage, strlen(clientMessage), 0);
    printf("[Client / INFO]: Sending message to server \"%s\"\n", clientMessage);

    // Получаем ответ от сервера
    ssize_t bytesRead = recv(clientSocket, responseBuffer, sizeof(responseBuffer), 0);
    if (bytesRead > 0) {
        printf("[Client / INFO]: Server responded \"%s\" \n", responseBuffer);
    } else {
        perror("[Client / ERROR]: Receive error");
    }


    close(clientSocket);
    return 0;
}
