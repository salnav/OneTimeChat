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
const int BUFFER_SIZE = 1024;

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


void error(const std::string& message) {
    std::cerr << message << std::endl;
    std::exit(EXIT_FAILURE);
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

string getIP(mongocxx::collection& coll, std::string field) {
    bsoncxx::stdx::optional<bsoncxx::document::value> result = coll.find_one({});
    if (result) {
        bsoncxx::document::element value = (*result)[field];
        cout << turnQueryResultIntoString(value) << endl;
        return turnQueryResultIntoString(value);
    }
    return "";
}

int main(int argc, char** argv) {
    mongocxx::options::client client_options;
    auto api = mongocxx::options::server_api(mongocxx::options::server_api::version::k_version_1);
    client_options.server_api_opts (api);
    mongocxx::client conn{uri, client_options};
    db = conn["Chats"];
    std::string code = std::string(argv[1]);
    coll = db[code];
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
    std::string IP = getIP(coll, "IP");
    
    if (inet_pton(AF_INET, IP.c_str(), &server_address.sin_addr) <= 0) {
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
