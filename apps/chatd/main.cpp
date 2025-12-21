#include <iostream>
#include <memory>

#include "NetBootstrap.h"
#include "Logger.h"
#include "Config.h"

int main() {
    Logger::init_minimal();
    LOG_INFO("Starting server...");

    Config::init(std::string(PROJECT_ROOT_DIR) + "/config.json");
    std::string log_level = Config::getString("logging.level", "debug");
    Logger::init_full(log_level);

    NetBootstrap net;
    net.start(8080);

    getchar(); // 阻塞，防止退出

    net.stop();
    return 0;
}





// #include "Logger.h"
// #include "Config.h"
// #include "NetBootstrap.h"
// #include "HttpCodec.h"
// #include "Dispatcher.h"

// #include <chrono>
// #include <thread>
// #include <vector>
// #include <iostream>

// int main() {
//     Logger::init_minimal();
//     LOG_INFO("Starting server...");

//     Config::init(std::string(PROJECT_ROOT_DIR) + "/config.json");
//     std::string log_level = Config::getString("logging.level", "debug");
//     Logger::init_full(log_level);

//     EventLoop loop;
    
//     sockaddr_in listenAddr{};
//     listenAddr.sin_family = AF_INET;
//     listenAddr.sin_port = htons(Config::getInt("server.port", 9000));
//     listenAddr.sin_addr.s_addr = INADDR_ANY;

//     NetBootstrap server(&loop, listenAddr);

//     HttpCodec httpCodec;

//     server.setReadCallback([](const std::shared_ptr<TcpConnection>& conn, Buffer& buf){
        
//     });

//     server.start();

//     return 0;
// }
