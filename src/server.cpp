#include <iostream>
#include "App.h"
#include "utils/dataModel.hpp"
#include "utils/mathlib.hpp"
#include <map>

void message_handler(
    uWS::WebSocket<false, true, WebSocketData> *ws,
    std::string_view message,
    uWS::OpCode opCode)
{
    std::cout << "Received message: " << message << std::endl;
    std::cout << "Received message length: " << message.length() << std::endl;
    std::cout << "Received message opCode: " << opCode << std::endl;

    std::cout << "Sending message..." << std::endl;
    Vector v = randomVector(16);
    for (int i = 0; i < v.size; i++)
    {
        std::cout << v.get(i) << " ";
    }
    std::cout << std::endl;
    std::string s = v.serialize();

    std::cout << "Sending message length: " << s.length() << std::endl;
    // std::cout << "Sending message: " << s << std::endl;
    for (int i = 0; i < s.length(); i++)
    {
        std::cout << (int)s[i] << " ";
    }
    std::cout << std::endl;

    ws->send(s, uWS::OpCode::TEXT, s.length() < 16 * 1024);

    // ws->send(message, opCode, message.length() < 16 * 1024);
}

std::map<int, int> ports;   // port - client id pair
std::map<int, int> clients; // client id - port pair
std::vector<int> client_ids;

class EntryServerHandler
{
public:
    int assign_new_port(int client_id)
    {
        // random between 9001 - 9999
        if (clients.find(client_id) != clients.end())
        {
            std::cout << "Client " << client_id << " already has a port assigned. Rejecting." << std::endl;
            return -1;
        }

        int port = 9001 + rand() % 999;
        while (ports.find(port) != ports.end())
        {
            port = 9001 + rand() % 999;
        }
        ports[port] = client_id;
        clients[client_id] = port;
        client_ids.push_back(client_id);
        return port;
    }

    void message_handler(
        uWS::WebSocket<false, true, WebSocketData> *ws,
        std::string_view message,
        uWS::OpCode opCode)
    {

        auto [client_id_str, op_type, data] = split_message(message);
        std::cout << "Received message: " << message << std::endl;
        std::cout << "Split message: " << client_id_str << " " << op_type << " " << data << std::endl;

        int client_id = std::stoi(std::string(client_id_str));
        std::cout << "Received " << op_type << " from client " << client_id << std::endl;

        int port = assign_new_port(client_id);
        std::cout << "Assigned port " << port << " to client " << client_id << std::endl;
        std::string response = std::string(client_id_str) + ";;ASSIGN_PORT;;" + std::to_string(port);

        ws->send(response, uWS::OpCode::TEXT, response.length() < 16 * 1024);
    }
};

class MainServerHandler
{
public:
    int port;
    int client_id;

    void open_handler(uWS::WebSocket<false, true, WebSocketData> *ws)
    {
        std::cout << "Client connected." << std::endl;
        ws->subscribe("broadcast");
    }

    void message_handler(
        uWS::WebSocket<false, true, WebSocketData> *ws,
        std::string_view message,
        uWS::OpCode opCode)
    {

        auto [client_id_str, op_type, data] = split_message(message);
        std::cout << "Received message: " << message << std::endl;
        std::cout << "Split message: " << client_id_str << " " << op_type << " " << data << std::endl;
    }
};

EntryServerHandler *open_entry_server()
{
    auto handler = new EntryServerHandler();
    uWS::App app =
        uWS::App()
            .ws<WebSocketData>(
                "/*",
                {
                    /* Settings */
                    .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
                    .maxPayloadLength = 100 * 1024 * 1024,
                    .idleTimeout = 16,
                    .maxBackpressure = 100 * 1024 * 1024,
                    .closeOnBackpressureLimit = false,
                    .resetIdleTimeoutOnSend = false,
                    .sendPingsAutomatically = true,
                    /* Handlers */
                    .upgrade = nullptr,
                    .open = [](auto * /*ws*/) {},
                    .message = [&handler](auto *ws, std::string_view message, uWS::OpCode opCode)
                    { handler->message_handler(ws, message, opCode); },
                    .dropped = [](auto * /*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/) {},
                    .drain = [](auto * /*ws*/) {},
                    .ping = [](auto * /*ws*/, std::string_view) {},
                    .pong = [](auto * /*ws*/, std::string_view) {},
                    .close = [](auto * /*ws*/, int /*code*/, std::string_view /*message*/) {},
                });

    app.listen(ENTRY_SERVER_PORT, [](auto *listen_socket)
               {if (listen_socket) {std::cout << "Listening on port " << ENTRY_SERVER_PORT << std::endl;} });

    app.run();
    return handler;
}

// uWS::App *servers[];
std::map<int, uWS::App *> servers;

// uWS::App *open_main_server(int port)
// {
//     if (servers.find(port) != servers.end() || servers[port] != nullptr)
//     {
//         std::cout << "Port " << port << " already has a server running. Rejecting." << std::endl;
//         return;
//     }

//     auto handler = new MainServerHandler();

//     uWS::App app =
//         uWS::App()
//             .ws<WebSocketData>(
//                 "/*",
//                 {
//                     /* Settings */
//                     .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
//                     .maxPayloadLength = 100 * 1024 * 1024,
//                     .idleTimeout = 16,
//                     .maxBackpressure = 100 * 1024 * 1024,
//                     .closeOnBackpressureLimit = false,
//                     .resetIdleTimeoutOnSend = false,
//                     .sendPingsAutomatically = true,
//                     /* Handlers */
//                     .upgrade = nullptr,
//                     .open = [](auto * /*ws*/) {},
//                     .message = [](auto *ws, std::string_view message, uWS::OpCode opCode)
//                     { message_handler(ws, message, opCode); },
//                     .dropped = [](auto * /*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/) {},
//                     .drain = [](auto * /*ws*/) {},
//                     .ping = [](auto * /*ws*/, std::string_view) {},
//                     .pong = [](auto * /*ws*/, std::string_view) {},
//                     .close = [](auto * /*ws*/, int /*code*/, std::string_view /*message*/) {},
//                 });

//     app.listen(port, [&port](auto *listen_socket)
//                {if (listen_socket) {std::cout << "Listening on port " << port << std::endl;} });

//     app.run();
//     return &app;
// }

void websocketDistributedMul(Matrix &A, Matrix &B, Matrix &C)
{

    // open servers with &C, task_count

    Vector rows[A.rows]; // A.
    Vector cols[B.cols]; // B.

    for (int i = 0; i < A.rows; i++)
    {
        rows[i] = A.getRow(i);
    }
    for (int i = 0; i < B.cols; i++)
    {
        cols[i] = B.getCol(i);
    }

    auto start = std::chrono::high_resolution_clock::now();

    int cur_client_idx = 0;

    for (int i = 0; i < A.rows; i++)
    {
        for (int j = 0; j < B.cols; j++)
        {
            int action_id = i * A.cols + j;

            int client_idx = cur_client_idx % CLIENT_COUNT;
            cur_client_idx = (cur_client_idx + 1) % CLIENT_COUNT;
            int client_id = client_ids[client_idx];

            auto server = servers[client_idx];
            std::string message;

            message = std::to_string(client_id) + ";;ASSIGN_ACTION;;" + std::to_string(action_id);
            server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);

            message = std::to_string(client_id) + ";;VECTOR_A;;" + rows[i].serialize();
            server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);

            message = std::to_string(client_id) + ";;VECTOR_B;;" + cols[j].serialize();
            server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);

            message = std::to_string(client_id) + ";;CALCULATE;;";
            server->publish("broadcast", message, uWS::OpCode::TEXT, message.length() < 16 * 1024);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
}

int main()
{
    srand(time(NULL));
    auto entry_server_handler = open_entry_server();
    return 0;

    for (int i = 0; i < 60; i++)
    {
        // wait for threads
        if (clients.size() == CLIENT_COUNT)
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // open servers with null pointers
    // servers = new uWS::App *[CLIENT_COUNT];
    for (int i = 0; i < CLIENT_COUNT; i++)
    {
        int port = clients[i];
        // servers[i] = open_main_server(port);
    }

    std::cout << "All " + std::to_string(CLIENT_COUNT) + " clients connected." << std::endl;

    std::cout << "Generate random matrix A and B\n";
    Matrix A = randomMatrix(SIZE, SIZE);
    Matrix B = randomMatrix(SIZE, SIZE);
    Matrix R(SIZE, SIZE);

    std::cout << "Start\n";
    websocketDistributedMul(A, B, R);

    // uWS::App app =
    //     uWS::App()
    //         .ws<WebSocketData>(
    //             "/*",
    //             {
    //                 /* Settings */
    //                 .compression = uWS::CompressOptions(uWS::DEDICATED_COMPRESSOR_4KB | uWS::DEDICATED_DECOMPRESSOR),
    //                 .maxPayloadLength = 100 * 1024 * 1024,
    //                 .idleTimeout = 16,
    //                 .maxBackpressure = 100 * 1024 * 1024,
    //                 .closeOnBackpressureLimit = false,
    //                 .resetIdleTimeoutOnSend = false,
    //                 .sendPingsAutomatically = true,
    //                 /* Handlers */
    //                 .upgrade = nullptr,
    //                 .open = [](auto * /*ws*/)
    //                 {
    //                                         /* Open event here, you may access ws->getUserData() which points to a PerSocketData struct */ },
    //                 .message = [](auto *ws, std::string_view message, uWS::OpCode opCode)
    //                 { message_handler(ws, message, opCode); },
    //                 .dropped = [](auto * /*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/)
    //                 {
    //         /* A message was dropped due to set maxBackpressure and closeOnBackpressureLimit limit */ },
    //                 .drain = [](auto * /*ws*/)
    //                 {
    //         /* Check ws->getBufferedAmount() here */ },
    //                 .ping = [](auto * /*ws*/, std::string_view)
    //                 {
    //         /* Not implemented yet */ },
    //                 .pong = [](auto * /*ws*/, std::string_view)
    //                 {
    //         /* Not implemented yet */ },
    //                 .close = [](auto * /*ws*/, int /*code*/, std::string_view /*message*/)
    //                 {
    //         /* You may access ws->getUserData() here */ },
    //             })
    //         .listen(9001, [](auto *listen_socket)
    //                 {
    //     if (listen_socket) {
    //         std::cout << "Listening on port " << 9001 << std::endl;
    //     } })
    //         .run();
}
