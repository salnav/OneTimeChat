#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <iostream>
#include <cstdlib>
#include <ifaddrs.h>

#define PORT 9034
#define PORTSTR "9034"   // Port we're listening on

int listener;     // Listening socket descriptor
int fd_count = 0;
struct pollfd *pfds;

std::string getIPAddress() {
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;

    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            char addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &pAddr->sin_addr, addr, sizeof(addr));
            std::string ipAddr(addr);
            if (ipAddr != "127.0.0.1") {
                freeifaddrs(addrs);
                return ipAddr;
            }
        }

        tmp = tmp->ifa_next;
    }

    freeifaddrs(addrs);
    return "";
}

// Get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// Return a listening socket
int get_listener_socket(void)
{
    int listener;     // Listening socket descriptor
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    int rv;

    struct addrinfo hints, *ai, *p;

    // Get us a socket and bind it
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORTSTR, &hints, &ai)) != 0) {
        fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
        exit(1);
    }
    
    for(p = ai; p != NULL; p = p->ai_next) {
        listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (listener < 0) { 
            continue;
        }
        
        // Lose the pesky "address already in use" error message
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

        if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
            close(listener);
            continue;
        }

        break;
    }

    freeaddrinfo(ai); // All done with this

    // If we got here, it means we didn't get bound
    if (p == NULL) {
        return -1;
    }

    // Listen
    if (listen(listener, 10) == -1) {
        return -1;
    }

    return listener;
}

// Add a new file descriptor to the set
void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size, int events)
{
    std::cout << "CONNECTING: " << newfd << std::endl;
    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        *pfds = (pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = events; // Check ready-to-read

    (*fd_count)++;
}

// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
}

int connect_to_server(const char* server_ip, int server_port) {
    struct sockaddr_in serv_addr;
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
        perror("connect");
        exit(1);
    }

    return sockfd;
}

void joinServer(std::string serverIP, int *fd_size) {
    // Create a client connection object
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(1);
    }

    // Connect the client to the server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, serverIP.c_str(), &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        exit(1);
    }
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        exit(1);
    }

    // Add the client to the pfds array
    add_to_pfds(&pfds, client_fd, &fd_count, fd_size, POLLIN);
}

in_addr_t getIPAddressAddr() {
    struct ifaddrs *addrs, *tmp;
    getifaddrs(&addrs);
    tmp = addrs;
    while (tmp) {
        if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
            if (strcmp(inet_ntoa(pAddr->sin_addr), "127.0.0.1") != 0) {
                in_addr_t ipAddr = inet_addr(inet_ntoa(pAddr->sin_addr));
                freeifaddrs(addrs);
                return ipAddr;
            }
        }
        tmp = tmp->ifa_next;
    }
    freeifaddrs(addrs);
    return INADDR_NONE;
}

// Main
int main(void)
{
    int newfd;        // Newly accept()ed socket descriptor
    struct sockaddr_storage remoteaddr; // Client address
    socklen_t addrlen;

    char buf[256];    // Buffer for client data

    char remoteIP[INET6_ADDRSTRLEN];

    // Start off with room for 5 connections
    // (We'll realloc as necessary)
    int fd_size = 5;
    pfds = (pollfd *)malloc(fd_size * sizeof(pollfd));

    // Set up and get a listening socket
    listener = get_listener_socket();

    if (listener == -1) {
        fprintf(stderr, "error getting listening socket\n");
        exit(1);
    }

    // Add the listener to set
    pfds[0].fd = listener;
    pfds[0].events = POLLIN; // Report ready to read on incoming connection

    fd_count = 1; // For the listener

    std::string IP = getIPAddress();

    std::cout << "Server listening on IP: " << IP << ", port: " << PORT << std::endl;

    joinServer(IP, &fd_size);

    // int sockfd = connect_to_server(IP.c_str(), PORT);
    // if (sockfd < 0) {
    //     perror("socket");
    //     exit(EXIT_FAILURE);
    // }

    // add_to_pfds(&pfds, sockfd, &fd_count, &fd_size, POLLOUT);

    // // Create a new socket for the server to use as a client
    // int server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // // Connect the socket to the chat server
    // struct sockaddr_in server_address;
    // server_address.sin_family = AF_INET;
    // server_address.sin_port = htons(PORT); // Replace with the port number of the chat server
    // server_address.sin_addr.s_addr = inet_addr(IP.c_str()); // Replace with the IP address of the chat server
    // connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address));

    // Now the server can send and receive messages like any other client
    // send(server_socket, "Hello, world!", 13, 0);
    // char buffer[1024];
    // recv(server_socket, buffer, 1024, 0);

    // add_to_pfds(&pfds, server_socket, &fd_count, &fd_size, POLLIN);

    // Main loop
    for(;;) {
        int poll_count = poll(pfds, fd_count, 1000);

        if (poll_count == -1) {
            perror("poll");
            exit(1);
        }

        // Run through the existing connections looking for data to read
        for(int i = 0; i < fd_count; i++) {

            // Check if someone's ready to read
            if (pfds[i].revents & POLLIN) { // We got one!!

                if (pfds[i].fd == listener) {
                    // If listener is ready to read, handle new connection

                    addrlen = sizeof remoteaddr;
                    newfd = accept(listener,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);

                    if (newfd == -1) {
                        perror("accept");
                    } else {
                        add_to_pfds(&pfds, newfd, &fd_count, &fd_size, POLLIN);
                        printf("pollserver: new connection from %s on "
                            "socket %d\n",
                            inet_ntop(remoteaddr.ss_family,
                                get_in_addr((struct sockaddr*)&remoteaddr),
                                remoteIP, INET6_ADDRSTRLEN),
                            newfd);
                    }
                } else {
                    // If not the listener, we're just a regular client
                    int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

                    int sender_fd = pfds[i].fd;
                    std::cout << "NBYTES: " << nbytes << std::endl;

                    if (nbytes <= 0) {
                        // Got error or connection closed by client
                        if (nbytes == 0) {
                            // Connection closed
                            printf("pollserver: socket %d hung up\n", sender_fd);
                        } else {
                            perror("recv");
                        }

                        close(pfds[i].fd); // Bye!

                        del_from_pfds(pfds, i, &fd_count);
                    } else {
                        // We got some good data from a client
                        char sender_name[50];
                        snprintf(sender_name, sizeof(sender_name), "Client %d: ", sender_fd); // Format the sender's name

                        for(int j = 0; j < fd_count; j++) {
                            // Send to everyone!
                            int dest_fd = pfds[j].fd;
                            if (dest_fd != listener && dest_fd != sender_fd) {
                                char message[1024];
                                snprintf(message, sizeof(message), "%s%s", sender_name, buf);
                                // printf("%s", message);
                                std::cout << message << std::endl;
                                if (send(dest_fd, message, nbytes + strlen(sender_name), 0) == -1) {
                                    perror("send");
                                }
                            } 
                            // else if (dest_fd == server_socket) {
                            //     // server socket has incoming data
                            //     char server_msg[1024];
                            //     int num_bytes = recv(dest_fd, server_msg, sizeof(server_msg), 0);
                            //     if (num_bytes > 0) {
                            //         // process the incoming message from the server
                            //         // for example:
                            //         printf("Received message from server: %s\n", server_msg);
                            //     }
                            // }
                        }
                    }
                } // END handle data from client
            } 
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
    
    return 0;
}



// #include <cstdint>
// #include <iostream>
// #include <vector>
// #include <thread>
// #include <dispatch/dispatch.h>
// #include <bsoncxx/json.hpp>
// #include <mongocxx/client.hpp>
// #include <mongocxx/stdx.hpp>
// #include <mongocxx/instance.hpp>
// #include <mongocxx/options/find.hpp>
// #include <mongocxx/uri.hpp>
// #include <mongocxx/options/client.hpp>
// #include <bsoncxx/builder/stream/helpers.hpp>
// #include <bsoncxx/builder/stream/document.hpp>
// #include <bsoncxx/builder/stream/array.hpp>
// #include <bsoncxx/exception/error_code.hpp>
// #include <QApplication>
// #include <QVBoxLayout>
// #include <QHBoxLayout>
// #include <QLineEdit>
// #include <QTextEdit>
// #include <QPushButton>
// #include <QLabel>
// #include <QMessageBox>
// #include <cstdlib> // for rand() and srand()
// #include <ctime> // for time()
// #include <random>
// #include <sstream>
// #include <string>

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <netdb.h>
// #include <poll.h>
// #include <ifaddrs.h>

// #define PORT 9034
// #define PORTSTR "9034"   // Port we're listening on

// int listener;     // Listening socket descriptor
// int fd_count = 0;
// struct pollfd *pfds;

// using bsoncxx::builder::stream::close_array;
// using bsoncxx::builder::stream::close_document;
// using bsoncxx::builder::stream::document;
// using bsoncxx::builder::stream::finalize;
// using bsoncxx::builder::stream::open_array;
// using bsoncxx::builder::stream::open_document;

// using namespace std;
// mongocxx::instance inst{};
// const auto uri = mongocxx::uri{"mongodb+srv://testableBoys:ABC@cluster0.oe7ufff.mongodb.net/?retryWrites=true&w=majority"};
// mongocxx::database db;
// mongocxx::collection coll;
// // mongocxx::change_stream stream;
// int user = 0;

// std::string turnQueryResultIntoString(bsoncxx::document::element queryResult) {

//     // check if no result for this query was found
//     if (!queryResult) {
//         return "[NO QUERY RESULT]";
//     }

//     // hax
//     bsoncxx::builder::basic::document basic_builder{};
//     basic_builder.append(bsoncxx::builder::basic::kvp("Your Query Result is the following value ", queryResult.get_value()));

//     // clean up resulting string
//     std::string rawResult = bsoncxx::to_json(basic_builder.view());
//     std::string frontPartRemoved = rawResult.substr(rawResult.find(":") + 2);
//     std::string backPartRemoved = frontPartRemoved.substr(0, frontPartRemoved.size() - 2);

//     return backPartRemoved;
// }


// void searchCollection(mongocxx::collection& coll, std::string field) {
//     bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one({});
//     if (result) {
//         bsoncxx::document::element value = (*result)[field];
//         cout << turnQueryResultIntoString(value) << endl;
//         std::cout << turnQueryResultIntoString(value) << std::endl;
//     }
    
// }

// // void updateCollection(mongocxx::collection& coll, std::string field) {
// //     int users = std::stoi(searchCollection(coll, field)); 

// //     if (users == 0) {
// //         cout << "NO USERS";
// //     }

// //     // coll.update_one(document{} << "users" << 10 << finalize,
// //     //                   document{} << "$set" << open_document <<
// //     //                     "i" << 110 << close_document << finalize);
// // }


// std::string getIPAddress() {
//     struct ifaddrs *addrs, *tmp;
//     getifaddrs(&addrs);
//     tmp = addrs;

//     while (tmp) {
//         if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
//             struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
//             char addr[INET_ADDRSTRLEN];
//             inet_ntop(AF_INET, &pAddr->sin_addr, addr, sizeof(addr));
//             std::string ipAddr(addr);
//             if (ipAddr != "127.0.0.1") {
//                 freeifaddrs(addrs);
//                 return ipAddr;
//             }
//         }

//         tmp = tmp->ifa_next;
//     }

//     freeifaddrs(addrs);
//     return "";
// }

// // Get sockaddr, IPv4 or IPv6:
// void *get_in_addr(struct sockaddr *sa)
// {
//     if (sa->sa_family == AF_INET) {
//         return &(((struct sockaddr_in*)sa)->sin_addr);
//     }

//     return &(((struct sockaddr_in6*)sa)->sin6_addr);
// }

// // Return a listening socket
// int get_listener_socket(void)
// {
//     int listener;     // Listening socket descriptor
//     int yes=1;        // For setsockopt() SO_REUSEADDR, below
//     int rv;

//     struct addrinfo hints, *ai, *p;

//     // Get us a socket and bind it
//     memset(&hints, 0, sizeof hints);
//     hints.ai_family = AF_UNSPEC;
//     hints.ai_socktype = SOCK_STREAM;
//     hints.ai_flags = AI_PASSIVE;
//     if ((rv = getaddrinfo(NULL, PORTSTR, &hints, &ai)) != 0) {
//         fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
//         exit(1);
//     }
    
//     for(p = ai; p != NULL; p = p->ai_next) {
//         listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
//         if (listener < 0) { 
//             continue;
//         }
        
//         // Lose the pesky "address already in use" error message
//         setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

//         if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
//             close(listener);
//             continue;
//         }

//         break;
//     }

//     freeaddrinfo(ai); // All done with this

//     // If we got here, it means we didn't get bound
//     if (p == NULL) {
//         return -1;
//     }

//     // Listen
//     if (listen(listener, 10) == -1) {
//         return -1;
//     }

//     return listener;
// }

// // Add a new file descriptor to the set
// void add_to_pfds(struct pollfd *pfds[], int newfd, int *fd_count, int *fd_size, int events)
// {
//     std::cout << "CONNECTING: " << newfd << std::endl;
//     // If we don't have room, add more space in the pfds array
//     if (*fd_count == *fd_size) {
//         *fd_size *= 2; // Double it

//         *pfds = (pollfd *)realloc(*pfds, sizeof(**pfds) * (*fd_size));
//     }

//     (*pfds)[*fd_count].fd = newfd;
//     (*pfds)[*fd_count].events = events; // Check ready-to-read

//     (*fd_count)++;
// }

// // Remove an index from the set
// void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
// {
//     // Copy the one from the end over this one
//     pfds[i] = pfds[*fd_count-1];

//     (*fd_count)--;
// }

// in_addr_t getIPAddressAddr() {
//     struct ifaddrs *addrs, *tmp;
//     getifaddrs(&addrs);
//     tmp = addrs;
//     while (tmp) {
//         if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET) {
//             struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
//             if (strcmp(inet_ntoa(pAddr->sin_addr), "127.0.0.1") != 0) {
//                 in_addr_t ipAddr = inet_addr(inet_ntoa(pAddr->sin_addr));
//                 freeifaddrs(addrs);
//                 return ipAddr;
//             }
//         }
//         tmp = tmp->ifa_next;
//     }
//     freeifaddrs(addrs);
//     return INADDR_NONE;
// }

// void joinServer(std::string serverIP) {
//     // Create a client connection object
//     int client_fd = socket(AF_INET, SOCK_STREAM, 0);
//     if (client_fd == -1) {
//         perror("socket");
//         exit(1);
//     }

//     // Connect the client to the server
//     struct sockaddr_in server_addr;
//     memset(&server_addr, 0, sizeof(server_addr));
//     server_addr.sin_family = AF_INET;
//     server_addr.sin_port = htons(PORT_NUMBER);
//     if (inet_pton(AF_INET, serverIP, &server_addr.sin_addr) <= 0) {
//         perror("inet_pton");
//         exit(1);
//     }
//     if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
//         perror("connect");
//         exit(1);
//     }

//     // Add the client to the pfds array
//     add_to_pfds(&pfds, client_fd, &fd_count, &fd_size, POLLIN);
// }

// void showChatWindow() {
//     QWidget *window = new QWidget;
//     window->resize(800, 600);
//     window->setMaximumSize(window->size());
//     QVBoxLayout *layout = new QVBoxLayout;

//     QTextEdit *chatText = new QTextEdit;
//     chatText->setReadOnly(true);
//     layout->addWidget(chatText);

//     QHBoxLayout *inputLayout = new QHBoxLayout;
//     QLineEdit *inputText = new QLineEdit;
//     QPushButton *sendButton = new QPushButton("Send");
//     inputLayout->addWidget(inputText);
//     inputLayout->addWidget(sendButton);
//     layout->addLayout(inputLayout);

//     window->setLayout(layout);
//     window->show();

//     QObject::connect(sendButton, &QPushButton::clicked, [inputText](){
//         QString message = inputText->text();
//         QString username = "You: ";
//         if(!message.isEmpty()){
//             // bsoncxx::document::value message_info = bsoncxx::builder::stream::document{} << "user" << user << "message" << message.toStdString() << bsoncxx::builder::stream::finalize;
//             // bsoncxx::document::view message_info_view = message_info.view();
//             // coll.update_one({}, document{} << "$push" << open_document << "messages" << message_info_view << close_document << finalize);

//             // chatText->append("<b>" + username + "</b>" + message);
//             // chatText->setAlignment(Qt::AlignRight);
//             // inputText->clear();
//         }
//     //     bsoncxx::document::value message_info = bsoncxx::builder::stream::document{} << "from" << 0 << "message" << "Hello?" << bsoncxx::builder::stream::finalize;
//     //    bsoncxx::document::view message_info_view = message_info.view();
//     //    coll.update_one({}, document{} << "$push" << open_document << "messages" << message_info_view << close_document << finalize);

//     });

//     // QPushButton *receiveButton = new QPushButton("Receive");
//     // inputLayout->addWidget(receiveButton);

//     // QObject::connect(receiveButton, &QPushButton::clicked, [chatText](){
//     //     QString message = "Hello, how are you?";
//     //     QString username = "Friend: ";

//     //     chatText->append("<b>" + username + "</b>" + message);
//     //     chatText->setAlignment(Qt::AlignLeft);
//     // });


//     std::thread watch_thread([]() {
//         mongocxx::options::change_stream options;
//         // Wait up to 1 second before polling again.
//         const std::chrono::milliseconds await_time{1000};
//         options.max_await_time(await_time);
//         const auto end = std::chrono::system_clock::now() + std::chrono::seconds{300};
//         mongocxx::change_stream stream = coll.watch(options);
        
//         while (std::chrono::system_clock::now() < end) {
//             for (const auto& event : stream) {
//                 std::cout << "STREAM IN THE CHAT: " << bsoncxx::to_json(event) << std::endl;
//                 // auto update_description = event["updateDescription"];
//                 // auto updated_fields = update_description["updatedFields"];
//                 // auto messages = updated_fields["messages"].get_array();

//                 // std::cout << "MESSAGES: " << bsoncxx::to_json(messages) << std::endl;     

//                 // auto messages_value = updated_fields["messages"];
//                 // if (messages_value && messages_value.type() == bsoncxx::type::k_array) {
//                 //     auto messages = messages_value.get_array().value;
//                 //     for (const auto& message : messages) {
//                 //         if (message.type() != bsoncxx::type::k_document) {
//                 //             std::cerr << "Invalid message type" << std::endl;
//                 //             continue;
//                 //         }
//                 //         auto message_view = message.get_document().view();
//                 //         auto message_text = message_view["text"].get_utf8_view().value();
//                 //         std::cout << "MESSAGE: " << message_text << std::endl;
//                 //     }

//                 // } else {
//                 //     std::cout << "No messages found" << std::endl;
//                 // }           
                
//                 // if (secured) {
//                 //     secured_flag = true;
//                 //     dispatch_async(dispatch_get_main_queue(), ^{
//                 //         // update the window's drag regions here
//                 //         user = 0;
//                 //         showChatWindow();
//                 //     });

//                 //     break;
//                 //     std::cout << "secured field is true" << std::endl;
//                 // } 
    
//                 // if (std::chrono::system_clock::now() >= end)
//                 //     break;
//             }
//         }
//     });

//     watch_thread.detach();
    
// }

// void showEnterCodeWindow() {
//     // Create a new widget
//     QWidget* window = new QWidget;
//     window->setWindowTitle("Chat Code");
//     window->setFixedSize(400, 300); // set the size of the window to 400x300

//     // Create a vertical layout
//     QVBoxLayout* layout = new QVBoxLayout;

//     // Create a horizontal layout for the "Chat code:" label and textbox
//     QHBoxLayout* hLayout = new QHBoxLayout;

//     // Create the "Chat code:" label
//     QLabel* codeLabel = new QLabel("Chat code:");
//     hLayout->addWidget(codeLabel);

//     // Create the textbox
//     QLineEdit* codeTextbox = new QLineEdit;
//     codeTextbox->setFixedSize(QSize(200, 40));
//     hLayout->addWidget(codeTextbox);

//     // Add the horizontal layout to the vertical layout
//     layout->addLayout(hLayout);

//     // Create the "Start Chatting" button
//     QPushButton* startChatButton = new QPushButton("Start Chatting");
//     startChatButton->setFixedSize(QSize(100, 40));
//     startChatButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black");
//     layout->addWidget(startChatButton);

//     QObject::connect(startChatButton, &QPushButton::clicked, [codeTextbox,window](){
//         QString codeText = codeTextbox->text();
//         std::string code = codeText.toStdString();

//         joinServer(code);

//         // coll = db[code];
//         // auto result = coll.update_one(document{} << "secured" << false << finalize,
//         //               document{} << "$set" << open_document <<
//         //                 "secured" << true << close_document << finalize);
//         // if (result) {
//         //     if (result->modified_count() == 1) {
//         //         std::cout << "SUCCESSFUL" << std::endl;
//         //         user = 1;
//         //         showChatWindow();
//         //         // The update was successful
//         //         // call the desired functions here
//         //     } else {
//         //         std::cout << "FAILED" << std::endl;
//         //         QMessageBox::warning( 
//         //         window, 
//         //         "OneTimeChat", 
//         //         "Code Invalid Or In Use Already" );
//         //     }
//         // } else {
//         //  // An error occurred while executing the update, check the exception that was thrown.
//         // }
//     });
        
    
//     // Set the layout for the widget
//     window->setLayout(layout);

//     // Show the widget
//     window->show();
// }

// void createServer() {
//     int newfd;        // Newly accept()ed socket descriptor
//     struct sockaddr_storage remoteaddr; // Client address
//     socklen_t addrlen;

//     char buf[256];    // Buffer for client data

//     char remoteIP[INET6_ADDRSTRLEN];

//     // Start off with room for 5 connections
//     // (We'll realloc as necessary)
//     int fd_size = 5;
//     pfds = (pollfd *)malloc(fd_size * sizeof(pollfd));

//     // Set up and get a listening socket
//     listener = get_listener_socket();

//     if (listener == -1) {
//         fprintf(stderr, "error getting listening socket\n");
//         exit(1);
//     }

//     // Add the listener to set
//     pfds[0].fd = listener;
//     pfds[0].events = POLLIN; // Report ready to read on incoming connection

//     fd_count = 1; // For the listener

//     std::string IP = getIPAddress();

//     std::cout << "Server listening on IP: " << IP << ", port: " << PORT << std::endl;

//     // Main loop
//     for(;;) {
//         int poll_count = poll(pfds, fd_count, 1000);

//         if (poll_count == -1) {
//             perror("poll");
//             exit(1);
//         }

//         // Run through the existing connections looking for data to read
//         for(int i = 0; i < fd_count; i++) {

//             // Check if someone's ready to read
//             if (pfds[i].revents & POLLIN) { // We got one!!

//                 // If not the listener, we're just a regular client
//                 int nbytes = recv(pfds[i].fd, buf, sizeof buf, 0);

//                 int sender_fd = pfds[i].fd;
//                 std::cout << "NBYTES: " << nbytes << std::endl;

//                 if (nbytes <= 0) {
//                     // Got error or connection closed by client
//                     if (nbytes == 0) {
//                         // Connection closed
//                         printf("pollserver: socket %d hung up\n", sender_fd);
//                     } else {
//                         perror("recv");
//                     }

//                     close(pfds[i].fd); // Bye!

//                     del_from_pfds(pfds, i, &fd_count);
//                 } else {
//                     // We got some good data from a client
//                     char sender_name[50];
//                     snprintf(sender_name, sizeof(sender_name), "Client %d: ", sender_fd); // Format the sender's name

//                     for(int j = 0; j < fd_count; j++) {
//                         // Send to everyone!
//                         int dest_fd = pfds[j].fd;
//                         if (dest_fd != listener && dest_fd != sender_fd) {
//                             char message[1024];
//                             snprintf(message, sizeof(message), "%s%s", sender_name, buf);
//                             // printf("%s", message);
//                             std::cout << message << std::endl;
//                             if (send(dest_fd, message, nbytes + strlen(sender_name), 0) == -1) {
//                                 perror("send");
//                             }
//                         } 
//                     }
//                 }
//             } 
//         } // END looping through file descriptors
//     } // END for(;;)--and you thought it would never end!
// }

// void showCodeWindow() {

//     createServer();

//     // Create a new widget
//     QWidget* window = new QWidget;
//     window->setWindowTitle("Chat Code");
//     window->setFixedSize(400, 300); // set the size of the window to 400x200

//     // // Create a vertical layout
//     QVBoxLayout* layout = new QVBoxLayout;

//     // // Create a horizontal layout
//     QHBoxLayout* hLayout = new QHBoxLayout;

//     // // Create a label for the "Chat code:" text
//     QLabel* chatCodeLabel = new QLabel("Chat code: ");
//     hLayout->addWidget(chatCodeLabel);

//     std::string code = getIPAddress();

//     // // Create the selectable label
//     QLabel* selectableLabel = new QLabel(QString::fromStdString(code));
//     selectableLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
//     selectableLabel->setAutoFillBackground(false);
//     selectableLabel->setStyleSheet("QLabel { background-color : transparent }");
//     selectableLabel->setFixedSize(100,30);
//     hLayout->addWidget(selectableLabel);

//     // // Add the horizontal layout to the vertical layout
//     layout->addLayout(hLayout);

//     // // Create the "Start Chatting" button
//     QPushButton* startChattingButton = new QPushButton("Start Chatting");
//     startChattingButton->setFixedSize(QSize(100, 40));
//     startChattingButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black");
           
//     showChatWindow();

//     QVBoxLayout* btnLayout = new QVBoxLayout;
//     btnLayout->addWidget(startChattingButton);
//     btnLayout->setAlignment(Qt::AlignCenter);
//     layout->addLayout(btnLayout,1);
//     layout->addWidget(startChattingButton);
//     layout->setAlignment(Qt::AlignCenter);
//     layout->addStretch();

//     // Set the layout for the widget
//     window->setLayout(layout);

//     // Show the widget
//     window->show();
   
// }

// void showWelcomeWindow() {
//         // Create a new widget
//     QWidget* window = new QWidget;
//     window->setWindowTitle("Welcome");
//     window->setFixedSize(400, 300); // set the size of the window to 400x300

//     // Create a vertical layout
//     QVBoxLayout* layout = new QVBoxLayout;
//     layout->setSpacing(10); // set the spacing between widgets in the layout to 20 pixels

//     // Create a label for the welcome text
//     QLabel* label = new QLabel("Welcome to One-Time Chat!");
//     label->setAlignment(Qt::AlignCenter);
//     label->setStyleSheet("font: 24pt;"); // Make the text bigger
//     layout->addWidget(label);

//     // Create the "New Chat" button
//     QPushButton* newChatButton = new QPushButton("New Chat");
//     newChatButton->setFixedSize(QSize(100, 40)); // make the button smaller
//     newChatButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black"); // Add a background color and rounded corners

//     QObject::connect(newChatButton, &QPushButton::clicked, [&](){
//         // window->hide();
//         showCodeWindow();
//     });

//     // Create the "Enter Code" button
//     QPushButton* enterCodeButton = new QPushButton("Enter Code");
//     enterCodeButton->setFixedSize(QSize(100, 40)); // make the button smaller
//     enterCodeButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black"); // Add a background color and rounded corners
   
//     QObject::connect(enterCodeButton, &QPushButton::clicked, [](){
//         // window->hide();
//         showEnterCodeWindow();
//     });

//     // Add the buttons to the layout
//     QVBoxLayout* btnLayout = new QVBoxLayout;
//     btnLayout->addWidget(newChatButton);
//     btnLayout->addWidget(enterCodeButton);
//     btnLayout->setAlignment(Qt::AlignCenter);
//     layout->setAlignment(Qt::AlignCenter);

//     layout->addStretch();
//     layout->addLayout(btnLayout,1);

//     // Set the layout for the widget
//     window->setLayout(layout);

//     // Show the widget
//     window->show();

// }

// // Main
// int main(void)
// {
//     QApplication a(argc, argv);
    
//     showWelcomeWindow();

//     return a.exec();

//     return 0;
// }
