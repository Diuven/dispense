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
}

class NodeHandler
{
public:
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
        return std::make_tuple("", "");
    }

    std::tuple<std::string, std::string> handle_enter_resp(std::string_view &data)
    {
        return std::make_tuple("", "");
    }

    std::string message_handler(const std::string &message, bool &stop)
    {
        auto [node_id_str, op_type, data] = split_message(message);
        std::cout << "Received operation: " << op_type << " with data: " << data << std::endl;

        std::string res_op_type = "", res_data = "";

        if (op_type == "STOP")
        {
            std::tie(res_op_type, res_data) = handle_stop(data);
            stop = true;
        }
        else if (op_type == "ENTER_RESP")
        {
            std::tie(res_op_type, res_data) = handle_enter_resp(data);
        }
        else
        {
            std::cout << "Invalid operation " << op_type << " with data: " << data << std::endl;
        }

        if (res_op_type == "")
            return "";
        else
        {
            std::string response = format_message(node_id, res_op_type, res_data);
            return response;
        }

        // if (op_type == "STOP")
        // {
        //     std::cout << "Received stop message. Stopping..." << std::endl;
        //     stop = true;
        //     return "";
        // }

        // if (op_type == "ASSIGN_ACTION")
        // {
        //     action_id = std::stoi(std::string(data));
        //     a = Vector();
        //     b = Vector();
        //     std::cout << "Assigned action: " << action_id << std::endl;
        //     return "";
        // }

        // if (op_type == "VECTOR_A")
        // {
        //     a = Vector().deserialize(std::string(data));
        //     std::cout << std::endl;
        //     return "";
        // }

        // if (op_type == "VECTOR_B")
        // {
        //     b = Vector().deserialize(std::string(data));
        //     std::cout << std::endl;
        //     return "";
        // }

        // if (op_type == "CALCULATE")
        // {
        //     if (a.size != b.size)
        //     {
        //         std::cout << "Vector sizes do not match. Exiting" << std::endl;
        //         stop = true;
        //         return "";
        //     }

        //     long long result = 0;
        //     for (int i = 0; i < a.size; i++)
        //     {
        //         result += a.get(i) * b.get(i);
        //     }

        //     std::string response = std::to_string(node_id) + ";;RESULT;;" + std::to_string(result);
        //     return response;
        // }

        // return "";
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
        ws->send(format_message(node_id, "CLOSE", ""));
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

    std::string message = format_message(node_id, "ENTER", "");
    manager.send_message(message);

    bool stop = false;
    while (!stop)
    {
        manager.ws->poll(-1);
        manager.ws->dispatch(
            [&stop, &handler](const std::string &message)
            {
                std::string response = handler.message_handler(message, stop);
                if (response != "")
                {
                    manager.send_message(response);
                }
            });
    }

    // int port = send_initialization(node_id);

    // std::cout << "Assigned port: " << port << std::endl;

    // if (port < 0)
    // {
    //     std::cout << "Failed to assign port. Exiting" << std::endl;
    //     return -1;
    // }

    // start_main_client(node_id, port);

    return 0;
}
