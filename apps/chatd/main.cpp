// 示例：main.cpp 使用方式
#include "Logger.h"
#include "Config.h"
#include "EventLoop.h"
#include "NetBootstrap.h"
#include "HttpService.h"
#include <memory>

int main() {
    // 初始化日志
    Logger::init_minimal();

    Config::init(std::string(PROJECT_ROOT_DIR) + "/config.json");
    std::string log_level = Config::getString("logging.level", "debug");
    Logger::init_full(log_level);
    
    int port = Config::getInt("server.port", 8080);
    std::string staticPath = Config::getString("server.static_path", "./public");
    
    // 创建事件循环
    auto loop = std::make_unique<EventLoop>();
    
    // 设置监听地址
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    
    // 创建HTTP服务
    auto httpService = std::make_shared<HttpService>(staticPath);
    
    // 注册自定义路由
    httpService->get("/api/hello", [](const std::shared_ptr<HttpRequest>& req,
                                      const std::shared_ptr<Session>& session) {
        nlohmann::json response = {
            {"message", "Hello, World!"},
            {"timestamp", time(nullptr)},
            {"client_ip", req->clientIp}
        };
        
        return HttpResponse::createJson(response.dump());
    });
    
    httpService->post("/api/users", [](const std::shared_ptr<HttpRequest>& req,
                                       const std::shared_ptr<Session>& session) {
        try {
            nlohmann::json body = nlohmann::json::parse(req->body);
            
            // 处理用户创建逻辑...
            nlohmann::json response = {
                {"id", 123},
                {"name", body.value("name", "")},
                {"status", "created"}
            };
            
            auto resp = HttpResponse::createJson(response.dump(), 
                                                HttpResponse::StatusCode::CREATED);
            return resp;
        } catch (const std::exception& e) {
            nlohmann::json error = {{"error", e.what()}};
            return HttpResponse::createJson(error.dump(), 
                                           HttpResponse::StatusCode::BAD_REQUEST);
        }
    });
    
    // 创建网络引导
    NetBootstrap bootstrap(loop.get(), addr);
    bootstrap.setReadCallback(
        [httpService](const TcpConnection::Ptr& conn, Buffer& buffer) {
            httpService->onMessage(conn, buffer);
        }
    );
    
    LOG_INFO("Server starting on port {}", port);
    LOG_INFO("Static file path: {}", staticPath);
    
    // 启动服务器
    bootstrap.start();
    
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
