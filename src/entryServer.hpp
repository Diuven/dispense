#include <iostream>
#include "App.h"

#ifndef DATA_MODEL_HPP
#include "utils/dataModel.hpp"
#endif
#ifndef MATHLIB_HPP
#include "utils/mathlib.hpp"
#endif
#include <fstream>

std::unordered_set<int> node_ids;

class EntryServerHandler
{
public:
    EntryServerHandler()
    {
    }

    std::string handle_enter(int node_id, std::string_view &data)
    {
        if (node_ids.count(node_id) > 0)
        {
            std::cout << "Client id " + std::to_string(node_id) + " is already taken.\n";
            return "0";
        }

        node_ids.insert(node_id);
        return "1";
    }

    std::string handle_close(int node_id, std::string_view &data)
    {
        if (node_ids.count(node_id) == 0)
        {
            std::cout << "Client id " + std::to_string(node_id) + " is not found.\n";
            return "0";
        }

        node_ids.erase(node_id);
        return "1";
    }

    // websocket handlers
    void message_handler(
        uWS::WebSocket<false, true, WebSocketData> *ws,
        std::string_view message,
        uWS::OpCode opCode)
    {
        auto [node_id_str, op_type, data] = split_message(message);
        int node_id = std::stoi(std::string(node_id_str));
        std::cout << "Received " << op_type << " , " << data << " from client " << node_id << std::endl;

        std::string res_op_type = std::string(op_type) + "_RESP";
        std::string res_data;

        if (op_type == "ENTER")
            res_data = handle_enter(node_id, data);
        else if (op_type == "CLOSE")
            res_data = handle_close(node_id, data);
        else
        {
            res_data = "Invalid operation";
            std::cout << "Invalid operation " << op_type << " from client " << node_id << std::endl;
            return;
        }

        std::cout << "Sending response " << res_op_type << " , " << res_data << " to client " << node_id << std::endl;
        std::string response = format_message(node_id, res_op_type, res_data);
        ws->send(response, uWS::OpCode::TEXT, response.length() < 16 * 1024);
    }

    void close_handler(
        uWS::WebSocket<false, true, WebSocketData> *ws,
        int code,
        std::string_view message)
    {
        std::cout << "Client disconnected. " << code << " " << message << std::endl;
    }

    // http handlers
    void get_stat_handler(uWS::HttpResponse<false> *res, uWS::HttpRequest *req)
    {
        res->end("Client count: " + std::to_string(node_ids.size()) + "\n");
    }
};

std::tuple<EntryServerHandler, uWS::App> get_entry_server()
{
    auto handler = new EntryServerHandler();
    uWS::App app =
        uWS::App()
            .get("/hello/:name",
                 [&handler](auto *res, auto *req)
                 { handler->get_stat_handler(res, req); })
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
                    .dropped = [](auto * /*ws*/, std::string_view /*message*/, uWS::OpCode /*opCode*/)
                    { std::cout << "Dropped message" << std::endl; },
                    .drain = [](auto * /*ws*/) {},
                    .ping = [](auto * /*ws*/, std::string_view)
                    { std::cout << "Ping received." << std::endl; },
                    .pong = [](auto * /*ws*/, std::string_view)
                    { std::cout << "Pong received." << std::endl; },
                    .close = [&handler](auto *ws, int code, std::string_view message)
                    { handler->close_handler(ws, code, message); },
                });

    app.listen(ENTRY_SERVER_PORT, [](auto *listen_socket)
               {if (listen_socket) {std::cout << "Listening on port " << ENTRY_SERVER_PORT << std::endl;} });

    return std::make_tuple(std::move(*handler), std::move(app));
}
