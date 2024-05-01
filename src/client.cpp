#include <string>
#include <iostream>
#include <thread>
#include "easywsclient.hpp"
#include "utils/mathlib.hpp"
#include "utils/dataModel.hpp"

#include <cstdlib>
#include <csignal>

bool connect_to_server(easywsclient::WebSocket::pointer &ws, const std::string &url)
{
    for (int i = 0; i < 30; i++)
    {
        try
        {
            ws = easywsclient::WebSocket::from_url(url);
            return true;
        }
        catch (const std::exception &e)
        {
            std::cout << "Failed to connect to server. Retrying..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    if (ws == nullptr)
    {
        std::cout << "Failed to connect to server. Exiting" << std::endl;
        return false;
    }
    return false;
}

class NodeHandler
{
public:
    bool stop = false;
    int node_id;

    int action_id;
    Vector a, b;
    NodeHandler(int node_id)
    {
        this->node_id = node_id;
    }

    std::tuple<std::string, std::string> make_enter()
    {
        return std::make_tuple("ENTER", "");
    }

    std::tuple<std::string, std::string> handle_stop(std::string_view &data)
    {
        std::cout << "Received stop message. Stopping..." << std::endl;
        stop = true;
        return std::make_tuple("", "");
    }

    std::tuple<std::string, std::string> handle_enter_resp(int got_action_id, std::string_view &data)
    {
        int success = std::stoi(std::string(data));
        if (success)
        {
            this->action_id = got_action_id;
            std::cout << "Successfully entered the network" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return std::make_tuple("NUDGE", "");
        }
        else
        {
            std::cout << "Failed to enter the network. Possibly port collision. Exiting..." << std::endl;
            stop = true;
            return std::make_tuple("", "");
        }
    }

    std::tuple<std::string, std::string> handle_nudge_resp(std::string_view &data)
    {
        int success = std::stoi(std::string(data));
        if (success)
        {
            std::cout << "Still in the network. Waiting..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return std::make_tuple("NUDGE", "");
        }
        else
        {
            std::cout << "Should not happen. Exiting..." << std::endl;
            stop = true;
            return std::make_tuple("", "");
        }
    }

    std::tuple<std::string, std::string> handle_assign_action(int got_action_id, std::string_view &data)
    {
        this->action_id = got_action_id;
        a = Vector();
        b = Vector();
        std::cout << "Assigned action: " << this->action_id << std::endl;
        return std::make_tuple("GET_A", "");
    }

    std::tuple<std::string, std::string> handle_get_a_resp(int got_action_id, std::string_view &data)
    {
        if (got_action_id != this->action_id)
        {
            std::cout << "Action ID mismatch in handle_get_a_resp. Exiting..." << std::endl;
            stop = true;
            return std::make_tuple("", "");
        }

        a = Vector().deserialize(std::string(data));
        std::cout << "Received vector A with size: " << a.size << std::endl;
        // for (int i = 0; i < a.size; i++)
        // {
        //     std::cout << a.get(i) << " ";
        // }
        // std::cout << std::endl;
        return std::make_tuple("GET_B", "");
    }

    std::tuple<std::string, std::string> handle_get_b_resp(int got_action_id, std::string_view &data)
    {
        if (got_action_id != this->action_id)
        {
            std::cout << "Action ID mismatch in handle_get_b_resp. Exiting..." << std::endl;
            stop = true;
            return std::make_tuple("", "");
        }

        b = Vector().deserialize(std::string(data));
        std::cout << "Received vector B with size: " << b.size << std::endl;
        // for (int i = 0; i < a.size; i++)
        // {
        //     std::cout << b.get(i) << " ";
        // }
        // std::cout << std::endl;

        long long result = a.dot(b);
        return std::make_tuple("RETURN", std::to_string(result));
    }

    std::string message_handler(const std::string &message)
    {
        auto [node_id_str, op_type, action_id_str, data] = split_message(message);
        int node_id = std::stoi(std::string(node_id_str));
        int got_action_id = (action_id_str.size() > 0) ? std::stoi(std::string(action_id_str)) : -1;
        std::cout << "Received operation: " << op_type << " of action id " << action_id << " with data length of : " << data.size() << std::endl;

        std::string res_op_type = "", res_data = "";

        if (op_type == "STOP")
            std::tie(res_op_type, res_data) = handle_stop(data);
        else if (op_type == "ENTER_RESP")
            std::tie(res_op_type, res_data) = handle_enter_resp(got_action_id, data);
        else if (op_type == "NUDGE_RESP")
            std::tie(res_op_type, res_data) = handle_nudge_resp(data);
        else if (op_type == "ASSIGN_ACTION")
            std::tie(res_op_type, res_data) = handle_assign_action(got_action_id, data);
        else if (op_type == "GET_A_RESP")
            std::tie(res_op_type, res_data) = handle_get_a_resp(got_action_id, data);
        else if (op_type == "GET_B_RESP")
            std::tie(res_op_type, res_data) = handle_get_b_resp(got_action_id, data);
        else
        {
            std::cout << "Invalid operation " << op_type << " with data: " << data << std::endl;
        }

        if (got_action_id > 0 && got_action_id != this->action_id)
        {
            std::cout << "Action ID mismatch in message_handler. Exiting..." << std::endl;
            stop = true;
        }

        if (res_op_type == "")
            return "";
        else
        {
            std::string response = format_message(
                node_id, res_op_type, this->action_id, res_data);
            return response;
        }
    }
};

class WebSocketManager
{
public:
    easywsclient::WebSocket::pointer ws;
    std::string url;
    int node_id;

    WebSocketManager(const std::string &url, int node_id)
    {
        this->url = url;
        this->node_id = node_id;
        ws = nullptr;
    }

    ~WebSocketManager()
    {
        close();
    }

    void close()
    {
        if (ws == nullptr)
            return;
        std::cout << "DESTRUCTOR!!! closing connection" << std::endl;
        ws->send(format_message(node_id, "CLOSE", -1, ""));
        ws->poll(-1); // recieve response?
        ws->close();
        delete ws;
    }
    bool connect()
    {
        bool res = connect_to_server(ws, url);
        if (!res)
        {
            std::cout << "Failed to connect to server. Exiting" << std::endl;
        }
        return res;
    }

    void send_message(const std::string &message)
    {
        std::cout << "Sending message: " << message << std::endl;
        ws->send(message);
    }

    bool is_closed()
    {
        return ws == nullptr || ws->getReadyState() == easywsclient::WebSocket::CLOSED;
    }
};

WebSocketManager manager = WebSocketManager("", 0);
void signal_handler(int signal)
{
    // graceful exit for the websocket
    std::cout << "Caught signal " << signal << std::endl;
    // manager.close();
    exit(signal);
}

int main()
{
    srand(time(NULL));
    int node_id = rand() % 1000;
    std::cout << "Client ID: " << node_id << std::endl;

    using easywsclient::WebSocket;
    NodeHandler handler(node_id);

    std::string url = "ws://" + SERVER_HOSTNAME + ":" + std::to_string(ENTRY_SERVER_PORT);

    manager = WebSocketManager(url, node_id);
    if (!manager.connect())
    {
        std::cout << "Failed to connect to server. Exiting" << std::endl;
        return 0;
    }

    std::signal(SIGINT, &signal_handler);
    std::signal(SIGINT, &signal_handler);
    std::signal(SIGABRT, &signal_handler);
    std::signal(SIGFPE, &signal_handler);
    std::signal(SIGILL, &signal_handler);
    std::signal(SIGTERM, &signal_handler);

    std::string message = format_message(node_id, "ENTER", -1, "");
    manager.send_message(message);

    while (!handler.stop && !manager.is_closed())
    {
        manager.ws->poll(-1);
        manager.ws->dispatch(
            [&handler](const std::string &message)
            {
                std::string response = handler.message_handler(message);
                if (response != "")
                {
                    manager.send_message(response);
                }
            });
    }

    return 0;
}
