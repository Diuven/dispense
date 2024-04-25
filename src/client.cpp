#include <string>
#include <iostream>
#include <thread>
#include "easywsclient.hpp"
#include "utils/mathlib.hpp"
#include "utils/dataModel.hpp"

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

int send_initialization(int client_id)
{
    // get the port

    using easywsclient::WebSocket;
    WebSocket::pointer ws = nullptr;
    std::string url = "ws://" + SERVER_HOSTNAME + ":" + std::to_string(ENTRY_SERVER_PORT);

    if (!connect_to_server(ws, url))
    {
        std::cout << "Failed to connect to server. Exiting" << std::endl;
        return -1;
    }

    std::cout << "Connected to server." << std::endl;
    std::cout << "Requesting port..." << std::endl;

    int port = -1;

    std::string message = std::to_string(client_id) + ";;REQUEST_PORT;;";
    ws->send(message);

    for (int i = 0; i < 30 && port < 0; i++)
    {
        ws->poll();
        ws->dispatch(
            [&port](const std::string &message)
            {
                std::cout << "Received: " << message << std::endl;
                auto [client_id_str, op_type, data] = split_message(message);
                port = std::stoi(std::string(data));
                std::cout << "Received port: " << port << std::endl;
            });
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    delete ws;

    return port;
}

class MainClientHandler
{
public:
    int client_id;
    int port;

    int action_id;
    Vector a, b;
    MainClientHandler(int client_id, int port)
    {
        this->client_id = client_id;
        this->port = port;
    }

    std::string message_handler(const std::string &message, bool &stop)
    {
        auto [client_id_str, op_type, data] = split_message(message);
        std::cout << "Received operation: " << op_type << std::endl;

        if (op_type == "STOP")
        {
            std::cout << "Received stop message. Stopping..." << std::endl;
            stop = true;
            return "";
        }

        if (op_type == "ASSIGN_ACTION")
        {
            action_id = std::stoi(std::string(data));
            a = Vector();
            b = Vector();
            std::cout << "Assigned action: " << action_id << std::endl;
            return "";
        }

        if (op_type == "VECTOR_A")
        {
            a = Vector().deserialize(std::string(data));
            std::cout << std::endl;
            return "";
        }

        if (op_type == "VECTOR_B")
        {
            b = Vector().deserialize(std::string(data));
            std::cout << std::endl;
            return "";
        }

        if (op_type == "CALCULATE")
        {
            if (a.size != b.size)
            {
                std::cout << "Vector sizes do not match. Exiting" << std::endl;
                stop = true;
                return "";
            }

            long long result = 0;
            for (int i = 0; i < a.size; i++)
            {
                result += a.get(i) * b.get(i);
            }

            std::string response = std::to_string(client_id) + ";;RESULT;;" + std::to_string(result);
            return response;
        }

        return "";
    }
};

void start_main_client(int client_id, int port)
{
    using easywsclient::WebSocket;
    MainClientHandler handler(client_id, port);

    WebSocket::pointer ws = nullptr;
    std::string url = "ws://" + SERVER_HOSTNAME + ":" + std::to_string(port);
    if (!connect_to_server(ws, url))
    {
        std::cout << "Failed to connect to server. Exiting" << std::endl;
        return;
    }

    std::cout << "Connected to server " + url << std::endl;

    // get instructions
    bool stop = false;
    while (!stop)
    {
        ws->poll();
        ws->dispatch(
            [&stop, &handler, &ws](const std::string &message)
            {
                std::string response = handler.message_handler(message, stop);
                if (response != "")
                {
                    ws->send(response);
                }
            });
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    delete ws;
}

int main()
{
    srand(time(NULL));
    int client_id = rand() % 1000;
    std::cout << "Client ID: " << client_id << std::endl;

    int port = send_initialization(client_id);

    std::cout << "Assigned port: " << port << std::endl;

    if (port < 0)
    {
        std::cout << "Failed to assign port. Exiting" << std::endl;
        return -1;
    }

    // start_main_client(client_id, port);

    return 0;

    // auto handle_message = [](const std::string &message)
    // {
    //     std::cout << "Received: " << message << std::endl;
    //     std::cout << "Received length: " << message.length() << std::endl;

    //     for (int i = 0; i < message.length(); i++)
    //     {
    //         std::cout << (int)message[i] << " ";
    //     }
    //     std::cout << std::endl;

    //     Vector v = Vector().deserialize(message);

    //     std::cout << "Deserialized: ";
    //     for (int i = 0; i < v.size; i++)
    //     {
    //         std::cout << v.get(i) << " ";
    //     }
    //     std::cout << std::endl;
    // };
    // using easywsclient::WebSocket;
    // WebSocket::pointer ws = WebSocket::from_url("ws://localhost:9001/");
    // ws->send("hello");

    // int count = 0;
    // while (true)
    // {
    //     // std::string message = "hello " + std::to_string(count++);
    //     ws->poll();
    //     // ws->send(message);
    //     ws->dispatch(handle_message);
    //     // wait 1 sec
    //     std::this_thread::sleep_for(std::chrono::seconds(1));
    // }
    // delete ws; // alternatively, use unique_ptr<> if you have C++11
    return 0;
}
