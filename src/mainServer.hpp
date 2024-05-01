#include <iostream>
#include "App.h"

#ifndef DATA_MODEL_HPP
#include "utils/dataModel.hpp"
#endif
#ifndef MATHLIB_HPP
#include "utils/mathlib.hpp"
#endif

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
