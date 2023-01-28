#include <cstdint>
#include <iostream>
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
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>


using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

int main(int argc, char *argv[]) {


// mongocxx::instance inst{};
// const auto uri = mongocxx::uri{"mongodb+srv://testableBoys:ABC@cluster0.oe7ufff.mongodb.net/?retryWrites=true&w=majority"};
// mongocxx::options::client client_options;
// auto api = mongocxx::options::server_api(mongocxx::options::server_api::version::k_version_1);
// client_options.server_api_opts (api);
// mongocxx::client conn{uri, client_options};
// mongocxx::database db = conn["test"];
// mongocxx::collection coll = db["test"];
// bsoncxx::document::value doc = bsoncxx::builder::stream::document{} << "name" << "MongoDB"
//   << "type" << "database"
//   << "count" << 1
//   << "versions" << bsoncxx::builder::stream::open_array
//     << "v3.2" << "v3.0" << "v2.6"
//   << close_array
//   << "info" << bsoncxx::builder::stream::open_document
//     << "x" << 203
//     << "y" << 102
//   << bsoncxx::builder::stream::close_document
//   << bsoncxx::builder::stream::finalize;
// coll.insert_one(doc.view());

QApplication a(argc, argv);

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

    QObject::connect(sendButton, &QPushButton::clicked, [inputText, chatText](){
        QString message = inputText->text();
        QString username = "You: ";
        if(!message.isEmpty()){

            chatText->append("<b>" + username + "</b>" + message);
            chatText->setAlignment(Qt::AlignRight);
            inputText->clear();
        }
    });

    QPushButton *receiveButton = new QPushButton("Receive");
    inputLayout->addWidget(receiveButton);

    QObject::connect(receiveButton, &QPushButton::clicked, [chatText](){
        QString message = "Hello, how are you?";
        QString username = "Friend: ";

        chatText->append("<b>" + username + "</b>" + message);
        chatText->setAlignment(Qt::AlignLeft);
    });

    return a.exec();

return 0;
}
