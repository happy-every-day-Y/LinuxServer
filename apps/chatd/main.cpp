#include "Logger.h"
#include "Config.h"
#include "NetBootstrap.h"

#include <chrono>
#include <thread>
#include <vector>

int main() {
    Logger::init_minimal();
    LOG_INFO("Starting server...");

    Config::init(std::string(PROJECT_ROOT_DIR) + "/config.json");
    std::string log_level = Config::getString("logging.level", "debug");
    Logger::init_full(log_level);

    EventLoop loop;
    
    sockaddr_in listenAddr{};
    listenAddr.sin_family = AF_INET;
    listenAddr.sin_port = htons(Config::getInt("server.port", 9000));
    listenAddr.sin_addr.s_addr = INADDR_ANY;

    NetBootstrap server(&loop, listenAddr);
    server.setMessageCallback([](const std::shared_ptr<TcpConnection>& c, Buffer& buf){
        std::string msg = buf.retrieveAllAsString();
        LOG_INFO("Echo: {}", msg);
        c->send(msg);
    });

    server.start();

    LOG_INFO("Server started on port {}", Config::getInt("server.port", 9000));

    LOG_INFO("All tasks completed.");

    LOG_DEBUG("This is a debug message");
    LOG_INFO("This is an info message");
    LOG_WARN("This is a warning message");
    LOG_ERROR("This is an error message");

    return 0;
}
