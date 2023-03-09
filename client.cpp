#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

const int BUFFER_SIZE = 1024;

void error(const std::string& message) {
    std::cerr << message << std::endl;
    std::exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        error("Usage: ./client [server IP address] [port number]");
    }

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        error("Failed to create socket");
    }

    sockaddr_in server_address;
    std::memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(std::atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0) {
        error("Invalid server IP address");
    }

    if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        error("Failed to connect to server");
    }

    std::string message;
    char buffer[BUFFER_SIZE];

    while (true) {
        std::cout << "Enter message: ";
        std::getline(std::cin, message);

        if (message == "quit") {
            break;
        }

        std::memset(buffer, 0, BUFFER_SIZE);
        std::strncpy(buffer, message.c_str(), sizeof(buffer));

        if (send(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
            error("Failed to send message");
        }

        std::memset(buffer, 0, BUFFER_SIZE);

        if (recv(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
            error("Failed to receive message");
        }

        std::cout << "Server: " << buffer << std::endl;
    }

    close(client_socket);

    return 0;
}
