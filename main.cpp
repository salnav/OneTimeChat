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
//             bsoncxx::document::value message_info = bsoncxx::builder::stream::document{} << "user" << user << "message" << message.toStdString() << bsoncxx::builder::stream::finalize;
//             bsoncxx::document::view message_info_view = message_info.view();
//             coll.update_one({}, document{} << "$push" << open_document << "messages" << message_info_view << close_document << finalize);

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

// std::string generateRandomCode() {
//     std::stringstream code;
//     std::random_device rd;
//     std::mt19937 gen(rd());
//     std::uniform_int_distribution<> dis(0, 9);

//     for (int i = 0; i < 10; ++i) {
//         int randomDigit = dis(gen);
//         code << randomDigit;
//     }
//     return code.str();
// }

// void pizza() {
//     std::cout << "Chicken";
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
//         coll = db[code];
//         auto result = coll.update_one(document{} << "secured" << false << finalize,
//                       document{} << "$set" << open_document <<
//                         "secured" << true << close_document << finalize);
//                         if (result) {
//         if (result->modified_count() == 1) {
//             std::cout << "SUCCESSFUL" << std::endl;
//             user = 1;
//             showChatWindow();
//             // The update was successful
//             // call the desired functions here
//         } else {
//             std::cout << "FAILED" << std::endl;
//             QMessageBox::warning( 
//             window, 
//             "OneTimeChat", 
//             "Code Invalid Or In Use Already" );
//         }
//         } else {
//         // An error occurred while executing the update, check the exception that was thrown.
//         }
//         });
        

    
//     // Set the layout for the widget
//     window->setLayout(layout);

//     // Show the widget
//     window->show();
// }

// void showCodeWindow() {
//     std::cout << "pizza" << std::endl;
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

//     std::string code =  generateRandomCode();
    
//     // bsoncxx::document::value doc = bsoncxx::builder::stream::document{} << "users" << 0 
//     // << bsoncxx::builder::stream::finalize;
//     // coll.insert_one(doc.view());

//     // // auto value = searchCollection(coll, "users");

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
//     coll = db[code];
//     bsoncxx::document::value doc = bsoncxx::builder::stream::document{} << "secured" << false << "messages" << bsoncxx::builder::stream::open_array << bsoncxx::builder::stream::close_array << bsoncxx::builder::stream::finalize;
//     coll.insert_one(doc.view());
 
//     // QObject::connect(startChattingButton, &QPushButton::clicked, [](){
       
//     //     coll.update_one(document{} << "secured" << false << finalize,
//     //                   document{} << "$set" << open_document <<
//     //                     "secured" << true << close_document << finalize);
//     // });

           
//     showChatWindow();
//             // document inserted successfully, continue with your code
   
//     // std::thread watch_thread([]() {
//     //     mongocxx::options::change_stream options;
//     //     // Wait up to 1 second before polling again.
//     //     bool secured_flag = false;
//     //     const std::chrono::milliseconds await_time{1000};
//     //     options.max_await_time(await_time);
//     //     const auto end = std::chrono::system_clock::now() + std::chrono::seconds{300};
//     //     mongocxx::change_stream stream = coll.watch(options);
        
//     //     while (std::chrono::system_clock::now() < end) {
//     //         for (const auto& event : stream) {
//     //             std::cout << "IN CODE WINDOW: " << bsoncxx::to_json(event) << std::endl;
//     //             auto update_description = event["updateDescription"];
//     //             auto updated_fields = update_description["updatedFields"];
//     //             auto secured = updated_fields["secured"].get_bool();
//     //             if (secured) {
//     //                 secured_flag = true;
//     //                 dispatch_async(dispatch_get_main_queue(), ^{
//     //             // update the window's drag regions here
//     //             user = 0;
//     //             showChatWindow();
//     //             });

//     //                 break;
//     //                 std::cout << "secured field is true" << std::endl;
//     //             } else {
//     //                 std::cout << "secured field is false" << std::endl;
//     //             }
    
//     //             if (std::chrono::system_clock::now() >= end)
//     //                 break;
//     //         }
//     //         if(secured_flag) {
//     //             break;
//     //         }
//     //     }
//     // });
//     // watch_thread.detach();
//     std::cout << "pizza2" << std::endl;


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

// int main(int argc, char *argv[]) {
//     QApplication a(argc, argv);
//     mongocxx::options::client client_options;
//     auto api = mongocxx::options::server_api(mongocxx::options::server_api::version::k_version_1);
//     client_options.server_api_opts (api);
//     mongocxx::client conn{uri, client_options};
//     db = conn["Chats"];
    
//     showWelcomeWindow();

//     return a.exec();

//     return 0;
// }

int main() {
    return 0;
}