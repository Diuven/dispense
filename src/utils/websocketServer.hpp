#include <string>
#include "App.h"
#include "dataModel.hpp"

class WebsocketServer
{
public:
    uWS::App app;

    WebsocketServer();
    ~WebsocketServer();

    void start();
    void stop();
    void send(const std::string &message);
};