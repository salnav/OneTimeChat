#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <random>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <bsoncxx/json.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/uri.hpp>
#include <mongocxx/options/client.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/exception/error_code.hpp>

#include "chat_utils.hpp"

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;



using namespace std;
mongocxx::instance inst{};
const auto uri = mongocxx::uri{"mongodb+srv://testableBoys:ABC@cluster0.oe7ufff.mongodb.net/?retryWrites=true&w=majority"};
mongocxx::database db;
mongocxx::collection coll;
// mongocxx::change_stream stream;
int user = 0;

// Constants
// const int PORT = 8080;
const int MAX_CLIENTS = 10;
const int BUFFER_SIZE = 1024;

// Function prototypes
int create_socket();
void bind_socket(int server_socket);
void handle_client(int client_socket);


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

int main() {

    // get ip
    std::string IP = getIPAddress();
    
    // Create server socket
    int server_socket = create_socket();

    // Bind server socket to port
    bind_socket(server_socket);

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



    // Listen for incoming connections
    listen(server_socket, MAX_CLIENTS);
    // std::cout << "Server listening on port " << PORT << std::endl;
    std::cout << "Server listening on IP: " << IP << ", port: " << PORT << std::endl;

    // Accept incoming connections and spawn thread to handle each client
    std::vector<std::thread> threads;
    while (true) {
        int client_socket = accept(server_socket, nullptr, nullptr);
        std::thread t(handle_client, client_socket);
        threads.push_back(std::move(t));
    }

    // Wait for threads to finish
    for (auto& t : threads) {
        t.join();
    }

    return 0;
}



int create_socket() {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }
    return server_socket;
}



void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string message;

    // Read incoming messages from client and print to console
    while (true) {
        int bytes_read = read(client_socket, buffer, BUFFER_SIZE - 1);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        message = buffer;
        std::cout << "Received message: " << message << std::endl;
    }

    // Close client socket
    close(client_socket);
}



void bind_socket(int server_socket) {
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    int bind_result = ::bind(server_socket, (struct sockaddr*) &server_address, sizeof(server_address));
    if (bind_result < 0) {
        std::cerr << "Failed to bind socket to port " << PORT << std::endl;
        exit(EXIT_FAILURE);
    }
}