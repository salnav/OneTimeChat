#include <cstdint>
#include <iostream>

#include <vector>
#include <thread>
#include <dispatch/dispatch.h>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/options/client.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/exception/error_code.hpp>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <cstdlib> // for rand() and srand()
#include <ctime> // for time()
#include <random>
#include <sstream>
#include <string>

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
#include <ifaddrs.h>
#include "MyLineEdit.h"

#define PORT 9034
#define PORTSTR "9034"   // Port we're listening on

int listener;     // Listening socket descriptor
int fd_count = 0;
struct pollfd *pfds;

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

// using namespace std;
mongocxx::instance inst{};
const auto uri = mongocxx::uri{"mongodb+srv://testableBoys:ABC@cluster0.oe7ufff.mongodb.net/?retryWrites=true&w=majority"};
mongocxx::database db;
mongocxx::collection coll;
int user = 0;

std::string generateRandomCode() {
    std::stringstream code;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 9);
    for (int i = 0; i < 10; ++i) {
        int randomDigit = dis(gen);
        code << randomDigit;
    }
    return code.str();
}

std::string turnQueryResultIntoString(bsoncxx::document::element queryResult) {

    // check if no result for this query was found
    if (!queryResult) {
        return "[NO QUERY RESULT]";
    }

    // hax
    bsoncxx::builder::basic::document basic_builder{};
    basic_builder.append(bsoncxx::builder::basic::kvp("Your Query Result is the following value ", queryResult.get_value()));

    // clean up resulting string
    std::string rawResult = bsoncxx::to_json(basic_builder.view());
    std::string frontPartRemoved = rawResult.substr(rawResult.find(":") + 2);
    std::string backPartRemoved = frontPartRemoved.substr(0, frontPartRemoved.size() - 2);

    return backPartRemoved;
}


void searchCollection(mongocxx::collection& coll, std::string field) {
    bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one({});
    if (result) {
        bsoncxx::document::element value = (*result)[field];
        // cout << turnQueryResultIntoString(value) << endl;
        std::cout << turnQueryResultIntoString(value) << std::endl;
    }
    
}

// void updateCollection(mongocxx::collection& coll, std::string field) {
//     int users = std::stoi(searchCollection(coll, field)); 

//     if (users == 0) {
//         cout << "NO USERS";
//     }

//     // coll.update_one(document{} << "users" << 10 << finalize,
//     //                   document{} << "$set" << open_document <<
//     //                     "i" << 110 << close_document << finalize);
// }


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

        if (::bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
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
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count, int *fd_size, int events)
{
    std::cout << "CONNECTING: " << newfd << std::endl;

    // If we don't have room, add more space in the pfds array
    if (*fd_count == *fd_size) {
        *fd_size *= 2; // Double it

        // Reallocate memory for the pfds array
        struct pollfd *temp = (struct pollfd *)realloc(*pfds, sizeof(struct pollfd) * (*fd_size));
        if (temp == NULL) {
            perror("realloc");
            exit(1);
        }
        *pfds = temp;
    }

    // Initialize the new pollfd struct
    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = events; // Check ready-to-read
    std::cout << "MIDDLE OF CONNECTING" << std::endl;

    // Increment the fd_count variable to reflect the new size of the pfds array
    (*fd_count)++;
    
    std::cout << "WE DONE CONNECTING" << std::endl;
}


// Remove an index from the set
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    // Copy the one from the end over this one
    pfds[i] = pfds[*fd_count-1];

    (*fd_count)--;
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

std::string getIPQuery(mongocxx::collection& coll, std::string field) {
    bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one({});
    if (result) {
        bsoncxx::document::element value = (*result)[field];
        return turnQueryResultIntoString(value);
    }
    return "";
}

std::string findIP(std::string code) {
    mongocxx::options::client client_options;
    auto api = mongocxx::options::server_api(mongocxx::options::server_api::version::k_version_1);
    client_options.server_api_opts (api);
    mongocxx::client conn{uri, client_options};
    db = conn["Chats"];
    coll = db[code];
    std::string unquoted = getIPQuery(coll, "IP");
    unquoted.erase(remove( unquoted.begin(), unquoted.end(), '\"' ),unquoted.end());
    return unquoted;
}

std::string bindCode() {
     std::string IP = getIPAddress();

    //Bind IP to random code in mongoDB
    std::string code =  generateRandomCode();
    mongocxx::options::client client_options;
    auto api = mongocxx::options::server_api(mongocxx::options::server_api::version::k_version_1);
    client_options.server_api_opts (api);
    mongocxx::client conn{uri, client_options};
    db = conn["Chats"];
    coll = db[code];
    bsoncxx::document::value doc = bsoncxx::builder::stream::document{} << "IP" << IP  << bsoncxx::builder::stream::finalize;
    coll.insert_one(doc.view());
    return code;
}

void createServer() {
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

                        std::cout << "GOT A MESSAGE!" << std::endl;

                        for(int j = 0; j < fd_count; j++) {
                            // Send to everyone!
                            int dest_fd = pfds[j].fd;
                            if (dest_fd != listener && dest_fd != sender_fd) {
                                char message[1024];
                                snprintf(message, sizeof(message), "%s%s", sender_name, buf);
                                // printf("%s", message);
                                // std::cout << message << std::endl;
                                if (send(dest_fd, message, nbytes + strlen(sender_name), 0) == -1) {
                                    perror("send");
                                }
                            } 
                        }
                        // Clear the contents of the buffer
                        memset(buf, 0, sizeof(buf));
                    }
                }

                
            } 
        } // END looping through file descriptors
    } // END for(;;)--and you thought it would never end!
}

int joinServer(std::string serverIP) {
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
    // add_to_pfds(&pfds, client_fd, &fd_count, fd_size, POLLIN);

    return client_fd;
}

void listenForMessages(int client_fd, QTextEdit *chatText) {
    while (true) {
        char buffer[1024];
        int nbytes = recv(client_fd, buffer, sizeof(buffer), 0);
        if (nbytes <= 0) {
            // Handle error or connection closed by server
            break;
        } else {
            // Process received message
            std::string message(buffer, nbytes);
            // Display the message or process it in some other way
            // std::cout << message << std::endl;

            // Get the position of the colon in the sender name
            std::size_t colon_pos = message.find(":");
            std::string sender_name = "";
            if (colon_pos != std::string::npos) {
                // Extract the sender name from the original string
                sender_name = "Recieved: ";
                message = message.substr(colon_pos+2);
            }

            chatText->append("<b>" + QString::fromStdString(sender_name) + "</b>" + QString::fromStdString(message));
            chatText->setAlignment(Qt::AlignLeft);
        }
    }
}

void showChatWindow(int client_socket) {
    QWidget *window = new QWidget;
    window->resize(800, 600);
    window->setMaximumSize(window->size());
    QVBoxLayout *layout = new QVBoxLayout;

    QTextEdit *chatText = new QTextEdit;
    chatText->setReadOnly(true);
    layout->addWidget(chatText);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    QLineEdit *inputText = new QLineEdit;
    QPushButton *sendButton = new QPushButton("Send");
    inputLayout->addWidget(inputText);
    inputLayout->addWidget(sendButton);
    layout->addLayout(inputLayout);

    window->setLayout(layout);
    window->show();

    std::thread client_thread([client_socket, chatText]() {
        listenForMessages(client_socket, chatText);
    });

    client_thread.detach();

    QObject::connect(sendButton, &QPushButton::clicked, [inputText, chatText, client_socket](){
        QString message = inputText->text();
        QString username = "Sent: ";
        if(!message.isEmpty()){
            // bsoncxx::document::value message_info = bsoncxx::builder::stream::document{} << "user" << user << "message" << message.toStdString() << bsoncxx::builder::stream::finalize;
            // bsoncxx::document::view message_info_view = message_info.view();
            // coll.update_one({}, document{} << "$push" << open_document << "messages" << message_info_view << close_document << finalize);

            // std::cout << "CLIENT " << client_socket << ": " << message.toStdString() << std::endl;
            send(client_socket, message.toStdString().c_str(), strlen(message.toStdString().c_str()), 0);

            chatText->append("<b>" + username + "</b>" + message);
            chatText->setAlignment(Qt::AlignRight);
            inputText->clear();
        }
    //     bsoncxx::document::value message_info = bsoncxx::builder::stream::document{} << "from" << 0 << "message" << "Hello?" << bsoncxx::builder::stream::finalize;
    //    bsoncxx::document::view message_info_view = message_info.view();
    //    coll.update_one({}, document{} << "$push" << open_document << "messages" << message_info_view << close_document << finalize);

    });
    
}

void showEnterCodeWindow() {
    // Create a new widget
    QWidget* window = new QWidget;
    window->setWindowTitle("Chat Code");
    window->setFixedSize(400, 300); // set the size of the window to 400x300

    // Create a vertical layout
    QVBoxLayout* layout = new QVBoxLayout;

    // Create a horizontal layout for the "Chat code:" label and textbox
    QHBoxLayout* hLayout = new QHBoxLayout;

    // Create the "Chat code:" label
    QLabel* codeLabel = new QLabel("Chat code:");
    hLayout->addWidget(codeLabel);

    // Create the textbox
    MyLineEdit* codeTextbox = new MyLineEdit;
    codeTextbox->setFixedSize(QSize(200, 40));
    hLayout->addWidget(codeTextbox);

    // Add the horizontal layout to the vertical layout
    layout->addLayout(hLayout);

    // Create the "Start Chatting" button
    QPushButton* startChatButton = new QPushButton("Start Chatting");
    startChatButton->setFixedSize(QSize(100, 40));
    startChatButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black");
    layout->addWidget(startChatButton);

    QObject::connect(codeTextbox, &MyLineEdit::clicked, [=]() {
        if (codeTextbox->text() == "Invalid Code") {
            codeTextbox->setStyleSheet("");
            codeTextbox->setText("");
        }
    });

    QObject::connect(startChatButton, &QPushButton::clicked, [codeTextbox](){
        QString codeText = codeTextbox->text();
        std::string code = codeText.toStdString();
        std::string IP = findIP(code);
        if (IP == "") {
            std::cout << "Invalid Code" << std::endl;
            codeTextbox->setText("Invalid Code");
            codeTextbox->setStyleSheet("color: red");
        } else {
            std::cout << "Connecting" << std::endl;
            int client_socket = joinServer(IP);
            showChatWindow(client_socket);
        }
        
    });
        
    
    // Set the layout for the widget
    window->setLayout(layout);

    // Show the widget
    window->show();
}

void showCodeWindow() {
    std::string bindedCode = bindCode();
    std::thread server_thread([]() {
        createServer();
    });

    server_thread.detach(); 

    // Create a new widget
    QWidget* window = new QWidget;
    window->setWindowTitle("Chat Code");
    window->setFixedSize(400, 300); // set the size of the window to 400x200

    // // Create a vertical layout
    QVBoxLayout* layout = new QVBoxLayout;

    // // Create a horizontal layout
    QHBoxLayout* hLayout = new QHBoxLayout;

    // // Create a label for the "Chat code:" text
    QLabel* chatCodeLabel = new QLabel("Chat code: ");
    hLayout->addWidget(chatCodeLabel);

    std::string code = getIPAddress();

    // Create the selectable label
    QLabel* selectableLabel = new QLabel(QString::fromStdString(bindedCode));
    selectableLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    selectableLabel->setAutoFillBackground(false);
    selectableLabel->setStyleSheet("QLabel { background-color : transparent }");
    selectableLabel->setFixedSize(100,30);
    hLayout->addWidget(selectableLabel);

    // Add the horizontal layout to the vertical layout
    layout->addLayout(hLayout);

    // Create the "Start Chatting" button
    QPushButton* startChattingButton = new QPushButton("Start Chatting");
    startChattingButton->setFixedSize(QSize(100, 40));
    startChattingButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black");
           
    QObject::connect(startChattingButton, &QPushButton::clicked, [code](){
        // window->hide();
        int client_socket = joinServer(code);
        showChatWindow(client_socket);
    });

    QVBoxLayout* btnLayout = new QVBoxLayout;
    btnLayout->addWidget(startChattingButton);
    btnLayout->setAlignment(Qt::AlignCenter);
    layout->addLayout(btnLayout,1);
    layout->addWidget(startChattingButton);
    layout->setAlignment(Qt::AlignCenter);
    layout->addStretch();

    // Set the layout for the widget
    window->setLayout(layout);

    // Show the widget
    window->show();
   
}

void showWelcomeWindow() {
        // Create a new widget
    QWidget* window = new QWidget;
    window->setWindowTitle("Welcome");
    window->setFixedSize(400, 300); // set the size of the window to 400x300

    // Create a vertical layout
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(10); // set the spacing between widgets in the layout to 20 pixels

    // Create a label for the welcome text
    QLabel* label = new QLabel("Welcome to One-Time Chat!");
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("font: 24pt;"); // Make the text bigger
    layout->addWidget(label);

    // Create the "New Chat" button
    QPushButton* newChatButton = new QPushButton("New Chat");
    newChatButton->setFixedSize(QSize(100, 40)); // make the button smaller
    newChatButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black"); // Add a background color and rounded corners

    QObject::connect(newChatButton, &QPushButton::clicked, [&](){
        // window->hide();
        showCodeWindow();
    });

    // Create the "Enter Code" button
    QPushButton* enterCodeButton = new QPushButton("Enter Code");
    enterCodeButton->setFixedSize(QSize(100, 40)); // make the button smaller
    enterCodeButton->setStyleSheet("background-color: #f0f0f0; border-radius: 10px; color: black"); // Add a background color and rounded corners
   
    QObject::connect(enterCodeButton, &QPushButton::clicked, [](){
        // window->hide();
        showEnterCodeWindow();
    });

    // Add the buttons to the layout
    QVBoxLayout* btnLayout = new QVBoxLayout;
    btnLayout->addWidget(newChatButton);
    btnLayout->addWidget(enterCodeButton);
    btnLayout->setAlignment(Qt::AlignCenter);
    layout->setAlignment(Qt::AlignCenter);

    layout->addStretch();
    layout->addLayout(btnLayout,1);

    // Set the layout for the widget
    window->setLayout(layout);

    // Show the widget
    window->show();

}

// Main
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    showWelcomeWindow();

    return a.exec();

    return 0;
}