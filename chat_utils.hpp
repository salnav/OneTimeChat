#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

// Define some constants
const int MESSAGE_LENGTH = 1024;
const int PORT = 8080;
const std::string ADDRESS = "127.0.0.1";

// Function to receive messages from the socket
void receive_message(int socket, char* message) {
    int bytes_received = recv(socket, message, MESSAGE_LENGTH, 0);
    message[bytes_received] = '\0'; // Add null-terminator to end of message
}

// Function to send messages to the socket
void send_message(int socket, const char* message) {
    send(socket, message, strlen(message), 0);
}
