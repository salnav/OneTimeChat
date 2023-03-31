// #include <iostream>
// #include <string>
// #include <thread>
// #include <chrono>
// #include <cstdlib>
// #include <cstring>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <unistd.h>

// const int BUFFER_SIZE = 1024;

// void error(const std::string& message) {
//     std::cerr << message << std::endl;
//     std::exit(EXIT_FAILURE);
// }

// int main(int argc, char** argv) {
//     if (argc != 3) {
//         error("Usage: ./client [server IP address] [port number]");
//     }

//     int client_socket = socket(AF_INET, SOCK_STREAM, 0);
//     if (client_socket == -1) {
//         error("Failed to create socket");
//     }

//     sockaddr_in server_address;
//     std::memset(&server_address, 0, sizeof(server_address));
//     server_address.sin_family = AF_INET;
//     server_address.sin_port = htons(std::atoi(argv[2]));
//     if (inet_pton(AF_INET, argv[1], &server_address.sin_addr) <= 0) {
//         error("Invalid server IP address");
//     }

//     if (connect(client_socket, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
//         error("Failed to connect to server");
//     }

//     // std::string message;
//     char buffer[BUFFER_SIZE];

//     // while (true) {
//     //     std::cout << "Enter message: ";
//     //     std::getline(std::cin, message);

//     //     if (message == "quit") {
//     //         break;
//     //     }

//     //     std::memset(buffer, 0, BUFFER_SIZE);
//     //     std::strncpy(buffer, message.c_str(), sizeof(buffer));

//     //     if (send(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
//     //         error("Failed to send message");
//     //     }

//     //     std::memset(buffer, 0, BUFFER_SIZE);

//     //     if (recv(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
//     //         error("Failed to receive message");
//     //     }

//     //     std::cout << "Server: " << buffer << std::endl;
//     // }

//     fd_set read_fds;
//     FD_ZERO(&read_fds);
//     FD_SET(STDIN_FILENO, &read_fds); // Add stdin to the set
//     FD_SET(client_socket, &read_fds); // Add the client socket to the set

//     while (true) {
//         // Wait for input from stdin or incoming message from server
//         fd_set tmp_fds = read_fds;
//         if (select(client_socket + 1, &tmp_fds, NULL, NULL, NULL) == -1) {
//             error("Failed to select()");
//         }

//         // Check if there's input from stdin
//         if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
//             std::string message;
//             std::getline(std::cin, message);
//             if (message == "quit") {
//                 break;
//             }
//             std::memset(buffer, 0, BUFFER_SIZE);
//             std::strncpy(buffer, message.c_str(), sizeof(buffer));
//             if (send(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
//                 error("Failed to send message");
//             }
//         }

//         // Check if there's incoming message from server
//         if (FD_ISSET(client_socket, &tmp_fds)) {
//             std::memset(buffer, 0, BUFFER_SIZE);
//             if (recv(client_socket, buffer, BUFFER_SIZE, 0) == -1) {
//                 error("Failed to receive message");
//             }
//             std::cout << "Server: " << buffer << std::endl;
//         }
//     }

//     close(client_socket);
// }

#include <iostream>
#include <string>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <fstream>
#include <sys/select.h>

using namespace std;
//Client side
int main(int argc, char *argv[])
{
    //we need 2 things: ip address and port number, in that order
    if(argc != 3)
    {
        cerr << "Usage: ip_address port" << endl; exit(0); 
    } //grab the IP address and port number 
    char *serverIp = argv[1]; int port = atoi(argv[2]); 
    //create a message buffer 
    // char msg[1500]; 
    //setup a socket and connection tools 
    struct hostent* host = gethostbyname(serverIp); 
    sockaddr_in sendSockAddr;   
    bzero((char*)&sendSockAddr, sizeof(sendSockAddr)); 
    sendSockAddr.sin_family = AF_INET; 
    sendSockAddr.sin_addr.s_addr = 
        inet_addr(inet_ntoa(*(struct in_addr*)*host->h_addr_list));
    sendSockAddr.sin_port = htons(port);
    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    //try to connect...
    int status = connect(client_sock,
                         (sockaddr*) &sendSockAddr, sizeof(sendSockAddr));
    if(status < 0)
    {
        cout<<"Error connecting to socket!"<<endl; 
    }
    cout << "Connected to the server!" << endl;
    int bytesRead = 0, bytesWritten = 0;
    struct timeval start1, end1;
    gettimeofday(&start1, NULL);

    // set client socket to non-blocking mode
    fcntl(client_sock, F_SETFL, O_NONBLOCK);

    // set up select parameters
    fd_set read_fds, write_fds;
    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    FD_SET(STDIN_FILENO, &read_fds);
    FD_SET(client_sock, &read_fds);

    // // main loop
    // while (true) {
    //     // wait for sockets to be ready for read or write
    //     select(std::max(STDIN_FILENO, client_sock) + 1, &read_fds, &write_fds, NULL, NULL);

    //     // handle standard input read event
    //     if (FD_ISSET(STDIN_FILENO, &read_fds)) {
    //         std::string message;
    //         getline(std::cin, message);
    //         if (message == "exit") {
    //             break;
    //         }
    //         FD_SET(client_sock, &write_fds);
    //     }

    //     // handle client socket read event
    //     if (FD_ISSET(client_sock, &read_fds)) {
    //         char buf[1024];
    //         int bytes_read = recv(client_sock, buf, sizeof(buf), 0);
    //         if (bytes_read > 0) {
    //             buf[bytes_read] = '\0';
    //             std::cout << "Received message from server: " << buf << std::endl;
    //         } else if (bytes_read == 0) {
    //             std::cout << "Server closed connection" << std::endl;
    //             break;
    //         }
    //     }

    //     // handle client socket write event
    //     if (FD_ISSET(client_sock, &write_fds)) {
    //         std::string message;
    //         getline(std::cin, message);
    //         if (message == "exit") {
    //             break;
    //         }
    //         message += '\n';
    //         send(client_sock, message.c_str(), message.size(), 0);
    //         FD_CLR(client_sock, &write_fds);
    //     }
    // }

    // main loop
    while (true) {
        // wait for sockets to be ready for read or write
        select(std::max(STDIN_FILENO, client_sock) + 1, &read_fds, &write_fds, NULL, NULL);

        // handle standard input read event
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            std::string message;
            getline(std::cin, message);
            if (message == "exit") {
                break;
            }
            FD_SET(client_sock, &write_fds);
        }

        // handle client socket read event
        if (FD_ISSET(client_sock, &read_fds)) {
            char buf[1024];
            int bytes_read = recv(client_sock, buf, sizeof(buf), 0);
            if (bytes_read > 0) {
                buf[bytes_read] = '\0';
                std::cout << "Received message from server: " << buf << std::endl;
            } else if (bytes_read == 0) {
                std::cout << "Server closed connection" << std::endl;
                break;
            }
        }

        // handle client socket write event
        if (FD_ISSET(client_sock, &write_fds)) {
            std::string message;
            getline(std::cin, message);
            if (message == "exit") {
                break;
            }
            message += '\n';
            send(client_sock, message.c_str(), message.size(), 0);
            FD_SET(client_sock, &read_fds);
            FD_CLR(client_sock, &write_fds);
        }
    }


    // while(1)
    // {
    //     cout << ">";
    //     string data;
    //     getline(cin, data);
    //     memset(&msg, 0, sizeof(msg));//clear the buffer
    //     strcpy(msg, data.c_str());
    //     if(data == "exit")
    //     {
    //         send(clientSd, (char*)&msg, strlen(msg), 0);
    //         break;
    //     }
    //     bytesWritten += send(clientSd, (char*)&msg, strlen(msg), 0);
    //     cout << "Awaiting server response..." << endl;
    //     memset(&msg, 0, sizeof(msg));//clear the buffer
    //     bytesRead += recv(clientSd, (char*)&msg, sizeof(msg), 0);
    //     if(!strcmp(msg, "exit"))
    //     {
    //         cout << "Server has quit the session" << endl;
    //         break;
    //     }
    //     cout << "Server: " << msg << endl;
    // }

    gettimeofday(&end1, NULL);
    close(client_sock);
    cout << "********Session********" << endl;
    cout << "Bytes written: " << bytesWritten << 
    " Bytes read: " << bytesRead << endl;
    cout << "Elapsed time: " << (end1.tv_sec- start1.tv_sec) 
      << " secs" << endl;
    cout << "Connection closed" << endl;
    return 0;    
}