// ===== MERGED SOURCE FILE =====
// Generated on Sat Dec 20 20:50:10 CST 2025

/***************************************/
/* FILE: ./CMakeLists.txt */
/***************************************/

add_subdirectory(base)
add_subdirectory(config)
add_subdirectory(net)
add_subdirectory(logic)
add_subdirectory(protocol)
add_subdirectory(service)


/***************************************/
/* FILE: ./all_code.cpp */
/***************************************/




/***************************************/
/* FILE: ./base/CMakeLists.txt */
/***************************************/

add_subdirectory(log)
add_subdirectory(thread_pool)


/***************************************/
/* FILE: ./base/log/CMakeLists.txt */
/***************************************/

add_library(log STATIC
    Logger.cpp
)

target_include_directories(log
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(log
    PUBLIC
        spdlog::spdlog
        project_options
)


/***************************************/
/* FILE: ./base/log/Logger.cpp */
/***************************************/

#include "Logger.h"
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

void Logger::init_minimal() {
    if (g_logger) return;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    g_logger = std::make_shared<spdlog::logger>("CHAT", console_sink);
    g_logger->set_level(spdlog::level::info);
    g_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
}

void Logger::init_full(const std::string& level_str) {
    spdlog::init_thread_pool(8192, 1);

    if (spdlog::get("CHAT")) {
        spdlog::drop("CHAT");
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/chat.log", 5 * 1024 * 1024, 3
    );

    spdlog::level::level_enum level = spdlog::level::debug;
    if (level_str == "info") level = spdlog::level::info;
    else if (level_str == "warn") level = spdlog::level::warn;
    else if (level_str == "error") level = spdlog::level::err;
    else if (level_str == "debug") level = spdlog::level::debug;

    g_logger = std::make_shared<spdlog::async_logger>(
        "CHAT",
        spdlog::sinks_init_list{console_sink, file_sink},
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
    );
    g_logger->set_level(level);
    g_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    spdlog::register_logger(g_logger);
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    return g_logger;
}



/***************************************/
/* FILE: ./base/log/Logger.h */
/***************************************/

#pragma once

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR "."
#endif

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

class Logger {
public:
    static void init_minimal();
    static void init_full(const std::string& level_str);
    static std::shared_ptr<spdlog::logger>& get();

private:
    static inline std::shared_ptr<spdlog::logger> g_logger = nullptr;
};

inline std::string trimFilePath(const char* fullPath) {
    std::string path(fullPath);
    std::string root(PROJECT_ROOT_DIR);
    if (path.find(root) == 0)
        path = path.substr(root.size() + 1);
    return path;
}

#define LOG_INFO(...)  Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...)  Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::err, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::debug, __VA_ARGS__)


/***************************************/
/* FILE: ./base/thread_pool/CMakeLists.txt */
/***************************************/

add_library(thread_pool STATIC
    Thread_pool.cpp
)

target_include_directories(thread_pool
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(thread_pool
    PUBLIC
        project_options
        log
        BS_thread_pool
)


/***************************************/
/* FILE: ./base/thread_pool/Thread_pool.cpp */
/***************************************/




/***************************************/
/* FILE: ./base/thread_pool/Thread_pool.h */
/***************************************/

// ThreadPool.h
#pragma once
#include <future>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include "BS_thread_pool.hpp"
#include "Logger.h"

class ThreadPool {
public:
    // 初始化线程池
    static void init(size_t threads = 0) {
        if (!pool_) {
            size_t n = threads == 0 ? std::thread::hardware_concurrency() : threads;
            pool_ = std::make_unique<BS::thread_pool<>>(n);
            LOG_INFO("ThreadPool initialized with {} threads", n);
        }
    }

    // 提交有返回值任务
    template<typename Func, typename... Args>
    static auto submit_task(Func&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        if (!pool_) init();
        try {
            return pool_->submit_task(std::forward<Func>(f), std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            LOG_ERROR("ThreadPool submit error: {}", e.what());
            throw;
        }
    }

    // 提交无返回值任务
    template<typename Func, typename... Args>
    static void detach_task(Func&& f, Args&&... args) {
        if (!pool_) init();
        try {
            pool_->detach_task(std::forward<Func>(f), std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            LOG_ERROR("ThreadPool detach error: {}", e.what());
            throw;
        }
    }

    // 等待所有任务完成
    static void wait() {
        if (pool_) pool_->wait();
    }

    // 获取当前线程池状态
    static size_t get_tasks_queued() {
        return pool_ ? pool_->get_tasks_queued() : 0;
    }

    static size_t get_tasks_running() {
        return pool_ ? pool_->get_tasks_running() : 0;
    }

    static size_t get_tasks_total() {
        return pool_ ? pool_->get_tasks_total() : 0;
    }

    static size_t get_thread_count() {
        return pool_ ? pool_->get_thread_count() : 0;
    }

private:
    ThreadPool() = default;
    ~ThreadPool() = default;

    static inline std::unique_ptr<BS::thread_pool<>> pool_ = nullptr;
};



/***************************************/
/* FILE: ./config/CMakeLists.txt */
/***************************************/

add_library(config STATIC Config.cpp)

target_include_directories(config PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(config
    PUBLIC
        project_options
        log
        nlohmann_json::nlohmann_json
)


/***************************************/
/* FILE: ./config/Config.cpp */
/***************************************/

#include "Config.h"
#include "Logger.h"
#include <fstream>

nlohmann::json Config::configJson;
std::unordered_map<std::string, std::string> Config::flatMap;

void Config::init(const std::string& filename) {
    LOG_INFO("Initializing configuration from '{}'", trimFilePath(filename.c_str()));

    std::ifstream f(filename);
    if (!f.is_open()) {
        LOG_ERROR("Cannot open config file: '{}'", trimFilePath(filename.c_str()));
        return;
    }

    try {
        f >> configJson;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config file '{}': {}", trimFilePath(filename.c_str()), e.what());
        return;
    }

    flatten(configJson);

    LOG_INFO("Configuration loaded successfully from '{}'", trimFilePath(filename.c_str()));
    LOG_INFO("Flattened keys:");
    for (const auto& kv : flatMap) {
        LOG_INFO("  {} = {}", kv.first, kv.second);
    }
}

void Config::flatten(const nlohmann::json& j, const std::string& prefix) {
    for (auto it = j.begin(); it != j.end(); ++it) {
        std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
        if (it->is_object()) {
            flatten(*it, key);
        } else {
            flatMap[key] = it->dump();
            if (it->is_string()) flatMap[key] = it->get<std::string>();
        }
    }
}

int Config::getInt(const std::string& key, int defaultValue) {
    if (flatMap.count(key))
        return std::stoi(flatMap[key]);
    return defaultValue;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    if (flatMap.count(key))
        return flatMap[key];
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    if (flatMap.count(key))
        return flatMap[key] == "true" || flatMap[key] == "1";
    return defaultValue;
}



/***************************************/
/* FILE: ./config/Config.h */
/***************************************/

#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>

class Config {
public:
    static void init(const std::string& filename);
    
    static int getInt(const std::string& key, int defaultValue = 0);
    static std::string getString(const std::string& key, const std::string& defaultValue = "");
    static bool getBool(const std::string& key, bool defaultValue = false);
    
private:
    static nlohmann::json configJson;
    static std::unordered_map<std::string, std::string> flatMap;
    
    static void flatten(const nlohmann::json& j, const std::string& prefix = "");
};



/***************************************/
/* FILE: ./dao/CMakeLists.txt */
/***************************************/

file(GLOB DAO_SOURCES *.cpp)

add_library(dao STATIC ${DAO_SOURCES})

target_include_directories(dao PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(dao
    PUBLIC
        project_options
        log
        config
)



/***************************************/
/* FILE: ./logic/CMakeLists.txt */
/***************************************/

add_subdirectory(handler)
add_subdirectory(dispatcher)


/***************************************/
/* FILE: ./logic/dispatcher/CMakeLists.txt */
/***************************************/

add_library(dispatcher STATIC
    Dispatcher.cpp)

target_include_directories(dispatcher
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(dispatcher
    PUBLIC
        project_options
        log
        codec
        session
        handler
        protocol
)


/***************************************/
/* FILE: ./logic/dispatcher/Dispatcher.cpp */
/***************************************/

// FILE: ./logic/dispatcher/Dispatcher.cpp
#include "Dispatcher.h"
#include "Logger.h"
#include "HttpHandler.h"
#include <algorithm>
#include <regex>

Dispatcher& Dispatcher::instance() {
    static Dispatcher instance;
    return instance;
}

void Dispatcher::registerHandler(std::shared_ptr<Handler> handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.push_back(handler);
    LOG_INFO("Registered handler: {}", handler->name());
}

void Dispatcher::registerHttpHandler(const std::string& name,
                                     const std::vector<std::string>& patterns,
                                     const std::vector<HttpRequest::Method>& methods,
                                     std::function<std::shared_ptr<HttpResponse>(
                                         const std::shared_ptr<HttpRequest>&,
                                         const std::shared_ptr<Session>&)> func) {
    auto handler = std::make_shared<HttpHandler>(name, patterns, methods, func);
    registerHandler(handler);
}

std::shared_ptr<HttpResponse> Dispatcher::dispatchHttp(
    const std::shared_ptr<HttpRequest>& request,
    const std::shared_ptr<Session>& session) {
    
    LOG_DEBUG("Dispatching HTTP request: {} {}", 
              request->path, 
              static_cast<int>(request->method));
    
    auto handler = findHandler(request);
    if (!handler) {
        LOG_WARN("No handler found for path: {}", request->path);
        
        if (m_default404Handler) {
            return m_default404Handler(request, session);
        }
        
        // 默认404响应
        auto resp = HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                        "<html><body><h1>404 Not Found</h1></body></html>",
                                        "text/html");
        return resp;
    }
    
    // 检查方法是否支持
    auto supportedMethods = handler->supportedMethods();
    if (std::find(supportedMethods.begin(), supportedMethods.end(), 
                  request->method) == supportedMethods.end()) {
        LOG_WARN("Method not allowed for path: {}", request->path);
        auto resp = HttpResponse::create(HttpResponse::StatusCode::METHOD_NOT_ALLOWED,
                                        "Method Not Allowed");
        resp->setHeader("Allow", "GET, POST, PUT, DELETE");
        return resp;
    }
    
    return handler->handleHttp(request, session);
}

void Dispatcher::dispatchWebSocket(
    const std::shared_ptr<WebSocketFrame>& frame,
    const std::shared_ptr<Session>& session) {
    
    LOG_DEBUG("Dispatching WebSocket frame, opcode: {}", 
              static_cast<int>(frame->opcode));
    
    // TODO: 实现WebSocket消息分发
    // 这里需要根据session或frame内容找到对应的处理器
    
    (void)frame;    // 消除未使用参数警告
    (void)session;  // 消除未使用参数警告
}

std::shared_ptr<Handler> Dispatcher::findHandler(
    const std::shared_ptr<HttpRequest>& request) {
    
    // 检查缓存
    std::string cacheKey = request->path + "_" + 
                          std::to_string(static_cast<int>(request->method));
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlerCache.find(cacheKey);
        if (it != m_handlerCache.end()) {
            return it->second;
        }
    }
    
    // 遍历所有处理器查找匹配
    for (const auto& handler : m_handlers) {
        if (matchAndParsePath(request, handler)) {
            // 缓存结果
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handlerCache[cacheKey] = handler;
            return handler;
        }
    }
    
    return nullptr;
}

void Dispatcher::addRoutePrefix(const std::string& prefix) {
    m_routePrefixes.push_back(prefix);
}

void Dispatcher::setDefault404Handler(
    std::function<std::shared_ptr<HttpResponse>(
        const std::shared_ptr<HttpRequest>&,
        const std::shared_ptr<Session>&)> handler) {
    m_default404Handler = std::move(handler);
}

bool Dispatcher::matchAndParsePath(const std::shared_ptr<HttpRequest>& request,
                                   const std::shared_ptr<Handler>& handler) const {
    
    // 首先检查处理器是否支持该HTTP方法
    auto supportedMethods = handler->supportedMethods();
    if (std::find(supportedMethods.begin(), supportedMethods.end(), 
                  request->method) == supportedMethods.end()) {
        return false;
    }
    
    // 检查路径模式
    for (const auto& pattern : handler->pathPatterns()) {
        // 应用路由前缀
        std::string fullPattern = pattern;
        for (const auto& prefix : m_routePrefixes) {
            if (request->path.find(prefix) == 0) {
                // 可能需要调整匹配逻辑
            }
        }
        
        // 简单字符串匹配（TODO: 可以改为正则表达式或参数化路径匹配）
        if (pattern == request->path || 
            (pattern.back() == '*' && request->path.find(pattern.substr(0, pattern.size() - 1)) == 0)) {
            return true;
        }
        
        // 尝试参数化路径匹配：/user/{id}
        if (pattern.find('{') != std::string::npos) {
            // 简化实现：将 {id} 替换为正则表达式 ([^/]+)
            std::regex regexPattern(std::regex_replace(pattern, 
                std::regex("\\{[^}]+\\}"), "([^/]+)"));
            
            if (std::regex_match(request->path, regexPattern)) {
                // TODO: 提取路径参数并设置到request中
                return true;
            }
        }
    }
    
    return false;
}


/***************************************/
/* FILE: ./logic/dispatcher/Dispatcher.h */
/***************************************/

// FILE: ./logic/dispatcher/Dispatcher.h
#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "Handler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebSocketFrame.h"
#include "Session.h"

class Dispatcher {
public:
    static Dispatcher& instance();
    
    // 注册处理器
    void registerHandler(std::shared_ptr<Handler> handler);
    
    // 注册HTTP处理器（快捷方式）
    void registerHttpHandler(const std::string& name,
                            const std::vector<std::string>& patterns,
                            const std::vector<HttpRequest::Method>& methods,
                            std::function<std::shared_ptr<HttpResponse>(
                                const std::shared_ptr<HttpRequest>&,
                                const std::shared_ptr<Session>&)> func);
    
    // 分发HTTP请求
    std::shared_ptr<HttpResponse> dispatchHttp(
        const std::shared_ptr<HttpRequest>& request,
        const std::shared_ptr<Session>& session);
    
    // 分发WebSocket消息
    void dispatchWebSocket(
        const std::shared_ptr<WebSocketFrame>& frame,
        const std::shared_ptr<Session>& session);
    
    // 查找匹配的处理器
    std::shared_ptr<Handler> findHandler(const std::shared_ptr<HttpRequest>& request);
    
    // 添加路由前缀（用于API版本控制等）
    void addRoutePrefix(const std::string& prefix);
    
    // 设置默认404处理器
    void setDefault404Handler(std::function<std::shared_ptr<HttpResponse>(
        const std::shared_ptr<HttpRequest>&,
        const std::shared_ptr<Session>&)> handler);
    
private:
    Dispatcher() = default;
    
    std::vector<std::shared_ptr<Handler>> m_handlers;
    std::vector<std::string> m_routePrefixes;
    std::unordered_map<std::string, std::shared_ptr<Handler>> m_handlerCache;
    mutable std::mutex m_mutex;
    
    std::function<std::shared_ptr<HttpResponse>(
        const std::shared_ptr<HttpRequest>&,
        const std::shared_ptr<Session>&)> m_default404Handler;
    
    // 匹配路径
    bool matchAndParsePath(const std::shared_ptr<HttpRequest>& request,
                          const std::shared_ptr<Handler>& handler) const;
};


/***************************************/
/* FILE: ./logic/handler/CMakeLists.txt */
/***************************************/

add_library(handler STATIC
    Handler.h HttpHandler.cpp)

target_include_directories(handler
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(handler
    PUBLIC
        project_options
        log
        connection
        protocol
        session
)


/***************************************/
/* FILE: ./logic/handler/Handler.h */
/***************************************/

// FILE: ./logic/handler/Handler.h
#pragma once
#include <memory>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebSocketFrame.h"
#include "Session.h"

class Handler {
public:
    virtual ~Handler() = default;
    
    // 处理HTTP请求
    virtual std::shared_ptr<HttpResponse> handleHttp(
        const std::shared_ptr<HttpRequest>& request,
        const std::shared_ptr<Session>& session) = 0;
    
    // 处理WebSocket消息
    virtual void handleWebSocket(
        const std::shared_ptr<WebSocketFrame>& frame,
        const std::shared_ptr<Session>& session) = 0;
    
    // 获取处理器名称
    virtual std::string name() const = 0;
    
    // 支持的HTTP方法
    virtual std::vector<HttpRequest::Method> supportedMethods() const {
        return {
            HttpRequest::Method::GET,
            HttpRequest::Method::POST
        };
    }
    
    // 支持的路径模式
    virtual std::vector<std::string> pathPatterns() const = 0;
    
    // 检查路径是否匹配
    virtual bool matchPath(const std::string& path) const {
        // 默认实现：检查是否匹配任何路径模式
        for (const auto& pattern : pathPatterns()) {
            if (pattern == path || 
                (pattern.back() == '*' && path.find(pattern.substr(0, pattern.size() - 1)) == 0)) {
                return true;
            }
        }
        return false;
    }
    
protected:
    // 路径匹配辅助方法
    static bool matchPattern(const std::string& pattern, const std::string& path,
                            std::unordered_map<std::string, std::string>& params) {
        // TODO: 实现参数化路径匹配
        return pattern == path;
    }
};


/***************************************/
/* FILE: ./logic/handler/HttpHandler.cpp */
/***************************************/

// FILE: ./logic/handler/HttpHandler.cpp
#include "HttpHandler.h"
#include "Logger.h"

HttpHandler::HttpHandler(const std::string& name,
                         const std::vector<std::string>& patterns,
                         const std::vector<HttpRequest::Method>& methods,
                         HttpHandlerFunc func)
    : m_name(name), m_patterns(patterns), m_methods(methods), m_handlerFunc(std::move(func)) {}

std::shared_ptr<HttpResponse> HttpHandler::handleHttp(
    const std::shared_ptr<HttpRequest>& request,
    const std::shared_ptr<Session>& session) {
    
    LOG_DEBUG("HttpHandler '{}' handling request: {} {}", 
              m_name, 
              request->path,
              request->getHeader("User-Agent"));
    
    try {
        return m_handlerFunc(request, session);
    } catch (const std::exception& e) {
        LOG_ERROR("HttpHandler '{}' error: {}", m_name, e.what());
        return HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                   "Internal Server Error: " + std::string(e.what()));
    }
}


/***************************************/
/* FILE: ./logic/handler/HttpHandler.h */
/***************************************/

// FILE: ./logic/handler/HttpHandler.h
#pragma once
#include "Handler.h"
#include <functional>
#include <string>
#include <unordered_map>

// HTTP处理器函数类型
using HttpHandlerFunc = std::function<std::shared_ptr<HttpResponse>(
    const std::shared_ptr<HttpRequest>&,
    const std::shared_ptr<Session>&)>;

class HttpHandler : public Handler {
public:
    HttpHandler(const std::string& name,
                const std::vector<std::string>& patterns,
                const std::vector<HttpRequest::Method>& methods,
                HttpHandlerFunc func);
    
    std::shared_ptr<HttpResponse> handleHttp(
        const std::shared_ptr<HttpRequest>& request,
        const std::shared_ptr<Session>& session) override;
    
    void handleWebSocket(
        const std::shared_ptr<WebSocketFrame>& frame,
        const std::shared_ptr<Session>& session) override {
        // HTTP处理器不处理WebSocket
        (void)frame;    // 消除未使用参数警告
        (void)session;  // 消除未使用参数警告
    }
    
    std::string name() const override { return m_name; }
    std::vector<std::string> pathPatterns() const override { return m_patterns; }
    std::vector<HttpRequest::Method> supportedMethods() const override { return m_methods; }
    
private:
    std::string m_name;
    std::vector<std::string> m_patterns;
    std::vector<HttpRequest::Method> m_methods;
    HttpHandlerFunc m_handlerFunc;
};


/***************************************/
/* FILE: ./model/CMakeLists.txt */
/***************************************/

file(GLOB MODEL_SOURCES *.cpp)

add_library(model STATIC ${MODEL_SOURCES})

target_include_directories(model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(model
    PUBLIC
        project_options
        log
)



/***************************************/
/* FILE: ./net/CMakeLists.txt */
/***************************************/

add_subdirectory(buffer)
add_subdirectory(reactor)
add_subdirectory(connection)
add_subdirectory(bootstrap)
add_subdirectory(acceptor)
add_subdirectory(session)


/***************************************/
/* FILE: ./net/acceptor/Acceptor.cpp */
/***************************************/

#include "Acceptor.h"
#include "Logger.h"
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int setNonBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return 0;
}

Acceptor::Acceptor(EventLoop *loop, const sockaddr_in &listenAddr)
    : m_loop(loop), m_listenFd(socket(AF_INET, SOCK_STREAM, 0))
{
    if(m_listenFd < 0){
        LOG_ERROR("Acceptor create socket failed: {}", strerror(errno));
        return;
    }

    setNonBlocking(m_listenFd);
    
    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    if(bind(m_listenFd, (sockaddr*)&listenAddr, sizeof(listenAddr)) < 0){
        LOG_ERROR("Acceptor bind failed: {}", strerror(errno));
        close(m_listenFd);
        return;
    }

    m_acceptChannel = std::make_unique<Channel>(m_loop, m_listenFd);
    m_acceptChannel->setReadCallback([this]{ handleRead(); });
}

Acceptor::~Acceptor()
{
    close(m_listenFd);
}

void Acceptor::listen()
{
    ::listen(m_listenFd, SOMAXCONN);
    m_acceptChannel->enableReading();
    LOG_INFO("Acceptor listening fd={}", m_listenFd);
}

void Acceptor::handleRead()
{
    sockaddr_in peerAddr{};
    socklen_t addrLen = sizeof(peerAddr);

    int connfd = ::accept(m_listenFd, (sockaddr*)&peerAddr, &addrLen);
    if(connfd >= 0) {
        setNonBlocking(connfd);
        LOG_INFO("Accepted new connection fd={} from {}:{}", 
                 connfd, inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));

        if(m_newConnectionCallback) {
            m_newConnectionCallback(connfd, peerAddr);
        }
    } else {
        LOG_ERROR("accept error: {}", strerror(errno));
    }
}



/***************************************/
/* FILE: ./net/acceptor/Acceptor.h */
/***************************************/

#pragma once
#include "EventLoop.h"
#include "Channel.h"
#include <functional>
#include <memory>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <cstring>

class Acceptor{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const sockaddr_in&)>;

    Acceptor(EventLoop* loop, const sockaddr_in& listenAddr);
    ~Acceptor();

    void listen();
    void setNewConnectionCallback(NewConnectionCallback cb) { m_newConnectionCallback = std::move(cb); }

private:
    void handleRead();

    EventLoop* m_loop;
    int m_listenFd;
    std::unique_ptr<Channel> m_acceptChannel;
    NewConnectionCallback m_newConnectionCallback;
};


/***************************************/
/* FILE: ./net/acceptor/CMakeLists.txt */
/***************************************/

add_library(acceptor STATIC
    Acceptor.cpp)

target_include_directories(acceptor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(acceptor
    PUBLIC
        project_options
        log
        reactor
)


/***************************************/
/* FILE: ./net/bootstrap/CMakeLists.txt */
/***************************************/

add_library(bootstrap STATIC
    NetBootstrap.cpp)

target_include_directories(bootstrap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(bootstrap
    PUBLIC
        project_options
        log
        reactor
        connection
        acceptor
        session
        protocol
        codec
)


/***************************************/
/* FILE: ./net/bootstrap/NetBootstrap.cpp */
/***************************************/

#include "NetBootstrap.h"
#include "Logger.h"

NetBootstrap::NetBootstrap(EventLoop *loop, const sockaddr_in &listenAddr)
    : m_loop(loop), m_acceptor(m_loop, listenAddr)
{
    m_acceptor.setNewConnectionCallback(
        [this](int fd, const sockaddr_in& addr){
            handleNewConnection(fd, addr);
        }
    );
}

void NetBootstrap::setReadCallback(ReadCallback cb)
{
    m_readCallback = std::move(cb);
}

void NetBootstrap::start()
{
    m_acceptor.listen();
    m_loop->loop();
}

void NetBootstrap::handleNewConnection(int fd, const sockaddr_in &addr)
{
    auto conn = std::make_shared<TcpConnection>(m_loop, fd);

    auto session = std::make_shared<Session>(conn);
    session->setPeerAddr(addr);
    SessionManager::addSession(fd, session);

    m_connections[fd] = conn;

    LOG_INFO("Session created, fd={}", fd);

    conn->setReadCallback(m_readCallback);
    conn->setCloseCallback([this](const std::shared_ptr<TcpConnection>& c){
        int fd = c->fd();

        LOG_INFO("Connection fd={} closed, removing from map", c->fd());

        SessionManager::removeSession(fd);
        m_connections.erase(fd);
    });

    LOG_INFO("New connection fd={} from {}:{}", fd,
             inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}



/***************************************/
/* FILE: ./net/bootstrap/NetBootstrap.h */
/***************************************/

#pragma once
#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include "Session.h"
#include "SessionManager.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <unordered_map>

class NetBootstrap{
public:
    using ReadCallback = TcpConnection::ReadCallback;

    NetBootstrap(EventLoop* loop, const sockaddr_in& listenAddr);
    void setReadCallback(ReadCallback cb);
    void start();

private:
    EventLoop* m_loop;
    Acceptor m_acceptor;
    ReadCallback m_readCallback;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> m_connections;

    void handleNewConnection(int fd, const sockaddr_in& addr);
};


/***************************************/
/* FILE: ./net/buffer/Buffer.cpp */
/***************************************/

#include "Buffer.h"
#include "Logger.h"
#include <unistd.h>
#include <sys/uio.h>
#include <algorithm>
#include <cerrno>
#include <cstring>

Buffer::Buffer(size_t initialSize)
    : m_buffer(initialSize), m_readerIndex(0), m_writerIndex(0) {}

size_t Buffer::readableBytes() const
{
    return m_writerIndex - m_readerIndex;
}

size_t Buffer::writableBytes() const
{
    return m_buffer.size() - m_writerIndex;
}

const char *Buffer::peek() const
{
    return m_buffer.data() + m_readerIndex;
}

void Buffer::retrieve(size_t len)
{
    if(len < readableBytes()){
        m_readerIndex += len;
    }else{
        retrieveAll();
    }
}

void Buffer::retrieveAll()
{
    m_readerIndex = m_writerIndex = 0;
}

std::string Buffer::retrieveAsString(size_t len)
{
    len = std::min(len, readableBytes());
    std::string result(peek(), len);
    retrieve(len);
    return result;   
}

std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes());
}

void Buffer::append(const char *data, size_t len)
{
    ensureWritableBytes(len);
    std::copy(data, data + len, m_buffer.begin() + m_writerIndex);
    m_writerIndex += len;
}

void Buffer::append(const std::string &str)
{
   append(str.data(), str.size());
}

ssize_t Buffer::readFd(int fd)
{
    char extraBuf[65536];
    struct iovec vec[2];

    vec[0].iov_base = m_buffer.data() + m_writerIndex;
    vec[0].iov_len  = writableBytes();
    vec[1].iov_base = extraBuf;
    vec[1].iov_len  = sizeof(extraBuf);

    const int iovcnt = writableBytes() < sizeof(extraBuf) ? 2 : 1;
    ssize_t n = readv(fd, vec, iovcnt);

    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            LOG_ERROR("Buffer::readFd fd={} error={}, msg={}", fd, errno, strerror(errno));
        }
        return n;
    }

    if (n == 0) {
        LOG_INFO("Buffer::readFd fd={} peer closed connection", fd);
        return 0;
    }

    if (static_cast<size_t>(n) <= writableBytes()) {
        m_writerIndex += n;
    } else {
        m_writerIndex = m_buffer.size();
        append(extraBuf, n - writableBytes());
    }
    return n;
}

ssize_t Buffer::writeFd(int fd)
{
    size_t nReadable = readableBytes();
    ssize_t n = ::write(fd, peek(), nReadable);

    if (n < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            LOG_ERROR("Buffer::writeFd fd={} error={}, msg={}", fd, errno, strerror(errno));
        }
        return n;
    }

    if (n == 0) {
        LOG_WARN("Buffer::writeFd fd={} write returned 0", fd);
        return 0;
    }

    retrieve(n);
    return n;
}

void Buffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() >= len) return;

    if (m_readerIndex + writableBytes() >= len) {
        size_t readable = readableBytes();
        std::copy(m_buffer.begin() + m_readerIndex,
                  m_buffer.begin() + m_writerIndex,
                  m_buffer.begin());
        m_readerIndex = 0;
        m_writerIndex = readable;

        LOG_DEBUG("Buffer compacted, new readableBytes={}", readable);
    } else {
        size_t oldSize = m_buffer.size();
        m_buffer.resize(m_writerIndex + len);

        LOG_DEBUG("Buffer resized from {} to {}", oldSize, m_buffer.size());
    }
}

// ========================== 新增方法 ==========================
size_t Buffer::findLastCompleteUtf8Char() const
{
    size_t len = readableBytes();
    if (len == 0) return 0;

    // pos 表示“到这里为止一定是完整的字节数”（即安全可取长度）
    size_t pos = len;

    // 从后往前跳过所有续字节（10xxxxxx）
    while (pos > 0 && ((peek()[pos - 1] & 0xC0) == 0x80)) {
        --pos;
    }

    // 如果 pos == 0，说明整个缓冲区都是续字节（非法情况），没有完整字符
    if (pos == 0) return 0;

    // 现在 pos - 1 是最后一个可能字符的起始字节
    unsigned char c = peek()[pos - 1];

    // 计算这个起始字节预期的字符长度
    size_t charLen = 1;
    if      ((c & 0x80) == 0x00) charLen = 1;  // ASCII
    else if ((c & 0xE0) == 0xC0) charLen = 2;
    else if ((c & 0xF0) == 0xE0) charLen = 3;
    else if ((c & 0xF8) == 0xF0) charLen = 4;
    else {
        // 非法起始字节（比如 0xF8+ 或孤立高位），保守当作1字节
        charLen = 1;
    }

    // 判断这个最后一个字符是否完整
    if (pos - 1 + charLen <= len) {
        return len;  // 完整，可以全取
    } else {
        return pos - 1;  // 不完整，取到上一个完整字符的结束位置（不包括当前起始字节）
    }
}

std::string Buffer::retrieveUtf8String()
{
    size_t safeLen = findLastCompleteUtf8Char();
    if (safeLen == 0) return "";

    std::string result(peek(), safeLen);
    retrieve(safeLen);
    return result;
}



/***************************************/
/* FILE: ./net/buffer/Buffer.h */
/***************************************/

#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

class Buffer{
public:
    explicit Buffer(size_t initialSize = 1024);

    size_t readableBytes() const;
    size_t writableBytes() const;

    const char* peek() const;
    void retrieve(size_t len);
    void retrieveAll();

    std::string retrieveAsString(size_t len);
    std::string retrieveAllAsString();
    std::string retrieveUtf8String();

    void append(const char* data, size_t len);
    void append(const std::string& str);

    ssize_t readFd(int fd);
    ssize_t writeFd(int fd);

private:
    void ensureWritableBytes(size_t len);
    size_t findLastCompleteUtf8Char() const;

private:
    std::vector<char> m_buffer;
    size_t m_readerIndex;
    size_t m_writerIndex;
};


/***************************************/
/* FILE: ./net/buffer/CMakeLists.txt */
/***************************************/

add_library(buffer STATIC
    Buffer.cpp)

target_include_directories(buffer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(buffer
    PUBLIC
        project_options
        log
)


/***************************************/
/* FILE: ./net/connection/CMakeLists.txt */
/***************************************/

add_library(connection STATIC
    TcpConnection.cpp)

target_include_directories(connection
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(connection
    PUBLIC
        project_options
        log
        buffer
        reactor
        dispatcher
        codec
        protocol
        handler
)


/***************************************/
/* FILE: ./net/connection/TcpConnection.cpp */
/***************************************/

#include "TcpConnection.h"
#include "Logger.h"

#include <unistd.h>
#include <errno.h>

TcpConnection::TcpConnection(EventLoop* loop, int fd)
    : m_loop(loop),
      m_channel(std::make_unique<Channel>(loop, fd)),
      m_inputBuffer(),
      m_outputBuffer(),
      m_state(Connecting)
{
    LOG_INFO("TcpConnection created fd={}", fd);

    m_channel->setReadCallback([this]() { handleRead(); });
    m_channel->setWriteCallback([this]() { handleWrite(); });
    m_channel->enableReading();

    m_state = Connected;
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection destroyed fd={}", fd());
    m_channel->disableAll();
    m_loop->removeChannel(m_channel.get());
}

void TcpConnection::handleRead()
{
    ssize_t n = m_inputBuffer.readFd(fd());

    if (n > 0) {
        if (m_readCallback) {
            m_readCallback(shared_from_this(), m_inputBuffer);
        }
    } else if (n == 0) {
        handleClose();
    } else {
        LOG_ERROR("TcpConnection handleRead error fd={}", fd());
        handleClose();
    }
}

void TcpConnection::handleWrite()
{
    ssize_t n = m_outputBuffer.writeFd(fd());

    if (n > 0) {
        if (m_outputBuffer.readableBytes() == 0) {
            m_channel->disableWriting();

            if (m_state == Disconnecting) {
                ::shutdown(fd(), SHUT_WR);
            }
        }
    } else if (n < 0) {
        LOG_ERROR("TcpConnection handleWrite error fd={}", fd());
        handleClose();
    }
}

void TcpConnection::handleClose()
{
    if (m_state == Closed) return;

    LOG_INFO("TcpConnection fd={} closed", fd());

    m_state = Closed;
    m_channel->disableAll();

    if (m_closeCallback) {
        m_closeCallback(shared_from_this());
    }
}

void TcpConnection::send(const std::string& data)
{
    if (m_state != Connected) return;

    sendInLoop(data);
}

void TcpConnection::shutdown()
{
    if (m_state == Connected) {
        m_state = Disconnecting;

        if (m_outputBuffer.readableBytes() == 0) {
            ::shutdown(fd(), SHUT_WR);
        }
    }
}

void TcpConnection::sendInLoop(const std::string& data)
{
    ssize_t n = 0;

    if (m_outputBuffer.readableBytes() == 0) {
        n = ::write(fd(), data.data(), data.size());
        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection write error fd={}", fd());
                handleClose();
                return;
            }
            n = 0;
        }
    }

    if (static_cast<size_t>(n) < data.size()) {
        m_outputBuffer.append(data.data() + n, data.size() - n);
        m_channel->enableWriting();
    }
}

int TcpConnection::fd() const
{
    return m_channel->fd();
}



/***************************************/
/* FILE: ./net/connection/TcpConnection.h */
/***************************************/

#pragma once

#include <memory>
#include <functional>
#include <string>
#include <sys/socket.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;

    using ConnectionCallback = std::function<void(const Ptr&)>;
    using ReadCallback       = std::function<void(const Ptr&, Buffer&)>;
    using CloseCallback      = std::function<void(const Ptr&)>;

    TcpConnection(EventLoop* loop, int fd);
    ~TcpConnection();

    void handleRead();
    void handleWrite();
    void handleClose();

    void send(const std::string& data);
    void shutdown();

    void setConnectionCallback(ConnectionCallback cb) { m_connectionCallback = std::move(cb); }
    void setReadCallback(ReadCallback cb)             { m_readCallback = std::move(cb); }
    void setCloseCallback(CloseCallback cb)           { m_closeCallback = std::move(cb); }

    int fd() const;

private:
    enum State {
        Connecting,
        Connected,
        Disconnecting,
        Closed
    };

    void sendInLoop(const std::string& data);

private:
    EventLoop* m_loop;
    std::unique_ptr<Channel> m_channel;

    Buffer m_inputBuffer;
    Buffer m_outputBuffer;

    State m_state;

    ConnectionCallback m_connectionCallback;
    ReadCallback       m_readCallback;
    CloseCallback      m_closeCallback;
};



/***************************************/
/* FILE: ./net/reactor/CMakeLists.txt */
/***************************************/

add_library(reactor STATIC
    EventLoop.cpp Channel.cpp EpollPoller.cpp)

target_include_directories(reactor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(reactor
    PUBLIC
        project_options
        log
)


/***************************************/
/* FILE: ./net/reactor/Channel.cpp */
/***************************************/

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include <sys/epoll.h>

Channel::Channel(EventLoop *loop, int fd)
: m_loop(loop), m_fd(fd), m_events(0), m_revents(0)
{
    LOG_DEBUG("Channel created, fd={}", m_fd);
}

Channel::~Channel()
{
    ::close(m_fd);
    LOG_DEBUG("Channel destroyed, fd={}", m_fd);
}

void Channel::handleEvent()
{
    LOG_DEBUG("Channel handleEvent, fd={}, revents={:#x}", m_fd, m_revents);

    if (m_revents & EPOLLIN) {
        LOG_DEBUG("Channel fd={} EPOLLIN triggered", m_fd);
        if (m_readCallback) {
            m_readCallback();
        } else {
            LOG_WARN("Channel fd={} EPOLLIN but readCallback is null", m_fd);
        }
    }

    if (m_revents & EPOLLOUT) {
        LOG_DEBUG("Channel fd={} EPOLLOUT triggered", m_fd);
        if (m_writeCallback) {
            m_writeCallback();
        } else {
            LOG_WARN("Channel fd={} EPOLLOUT but writeCallback is null", m_fd);
        }
    }
}

void Channel::setReadCallback(EventCallback cb)
{
    m_readCallback = std::move(cb);
    LOG_DEBUG("Channel fd={} set read callback", m_fd);
}

void Channel::setWriteCallback(EventCallback cb)
{
    m_writeCallback = std::move(cb);
    LOG_DEBUG("Channel fd={} set write callback", m_fd);
}

int Channel::fd() const
{
    return m_fd;
}

uint32_t Channel::events() const
{
    return m_events;
}

void Channel::setRevents(uint32_t revt)
{
    m_revents = revt;
}

void Channel::enableReading()
{
    if (!(m_events & EPOLLIN)) {
        m_events |= EPOLLIN;
        LOG_DEBUG("Channel fd={} enableReading", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::enableWriting()
{
    if (!(m_events & EPOLLOUT)) {
        m_events |= EPOLLOUT;
        LOG_DEBUG("Channel fd={} enableWriting", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::disableReading()
{
    if (m_events & EPOLLIN) {
        m_events &= ~EPOLLIN;
        LOG_DEBUG("Channel fd={} disableReading", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::disableWriting()
{
    if (m_events & EPOLLOUT) {
        m_events &= ~EPOLLOUT;
        LOG_DEBUG("Channel fd={} disableWriting", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::disableAll()
{
    if (m_events != 0) {
        m_events = 0;
        LOG_DEBUG("Channel fd={} disableAll", m_fd);
        m_loop->updateChannel(this);
    }
}


/***************************************/
/* FILE: ./net/reactor/Channel.h */
/***************************************/

#pragma once
#include <functional>
#include <cstdint>
#include <sys/socket.h>
#include <unistd.h>

class EventLoop;

class Channel{
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent();

    void setReadCallback(EventCallback cb);
    void setWriteCallback(EventCallback cb);
    
    int fd() const;
    uint32_t events() const;
    void setRevents(uint32_t revt);

    void enableReading();
    void enableWriting();
    void disableReading();
    void disableWriting();
    void disableAll();

private:
    EventLoop* m_loop;
    const int m_fd;

    uint32_t m_events;
    uint32_t m_revents;

    EventCallback m_readCallback;
    EventCallback m_writeCallback;
};


/***************************************/
/* FILE: ./net/reactor/EpollPoller.cpp */
/***************************************/

#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>

static const int kMaxEvents = 1024;

EpollPoller::EpollPoller()
{
    m_epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (m_epollfd < 0) {
        LOG_ERROR("epoll_create1 failed: {}", strerror(errno));
    } else {
        LOG_INFO("EpollPoller created, epollfd={}", m_epollfd);
    }
}

EpollPoller::~EpollPoller()
{
    LOG_INFO("EpollPoller destroyed, epollfd={}", m_epollfd);
    close(m_epollfd);
}

void EpollPoller::poll(int timeoutMs, std::vector<Channel *> &activeChannels)
{
    epoll_event events[kMaxEvents];
    int num = epoll_wait(m_epollfd, events, kMaxEvents, timeoutMs);
    if (num < 0) {
        LOG_ERROR("epoll_wait error: {}", strerror(errno));
        return;
    }

    //LOG_DEBUG("epoll_wait returned {} events", num);

    for (int i = 0; i < num; i++) {
        Channel *channel = static_cast<Channel *>(events[i].data.ptr);
        channel->setRevents(events[i].events);

        LOG_DEBUG("Active fd={}, events={:#x}", channel->fd(), static_cast<uint32_t>(events[i].events));

        activeChannels.push_back(channel);
    }
}

void EpollPoller::updateChannel(Channel *channel)
{
    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = channel->events();
    ev.data.ptr = channel;

    int fd = channel->fd();

    if (m_channels.count(fd) == 0) {
        if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            LOG_ERROR("epoll_ctl ADD failed, fd={}, err={}", fd, strerror(errno));
            return;
        }
        m_channels[fd] = channel;
        LOG_DEBUG("epoll ADD fd={}, events={:#x}", fd, static_cast<uint32_t>(ev.events));
    } else {
        if (epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
            LOG_ERROR("epoll_ctl MOD failed, fd={}, err={}", fd, strerror(errno));
            return;
        }
        LOG_DEBUG("epoll MOD fd={}, events={:#x}", fd, static_cast<uint32_t>(ev.events));
    }
}

void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    if (m_channels.count(fd)) {
        epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
        m_channels.erase(fd);
        LOG_INFO("epoll DEL fd={}", fd);
    }
}



/***************************************/
/* FILE: ./net/reactor/EpollPoller.h */
/***************************************/

#pragma once
#include <vector>
#include <unordered_map>

class Channel;

class EpollPoller{
public:
    EpollPoller();
    ~EpollPoller();
    
    void poll(int timeoutMs, std::vector<Channel*>& activeChannels);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    int m_epollfd;
    std::unordered_map<int, Channel*> m_channels;
};


/***************************************/
/* FILE: ./net/reactor/EventLoop.cpp */
/***************************************/

#include "EventLoop.h"
#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"

EventLoop::EventLoop()
: m_quit(false), m_poller(new EpollPoller())
{
    LOG_INFO("EventLoop created");
}

EventLoop::~EventLoop()
{
    LOG_INFO("EventLoop destroyed");
}
void EventLoop::loop()
{
    LOG_INFO("EventLoop loop started");

    while(!m_quit){
        m_activeChannels.clear();

        //LOG_DEBUG("EventLoop polling...");
        m_poller->poll(1000, m_activeChannels);

        //LOG_DEBUG("EventLoop got {} active channels", m_activeChannels.size());

        for(Channel* ch : m_activeChannels){
            //LOG_DEBUG("EventLoop handling fd={}", ch->fd());
            ch->handleEvent();
        }
    }

    LOG_INFO("EventLoop loop exited");
}

void EventLoop::updateChannel(Channel *channel)
{
    LOG_DEBUG("EventLoop updateChannel fd={}", channel->fd());
    m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    LOG_DEBUG("EventLoop removeChannel fd={}", channel->fd());
    m_poller->removeChannel(channel);
}



/***************************************/
/* FILE: ./net/reactor/EventLoop.h */
/***************************************/

#pragma once
#include <vector>
#include <memory>

class Channel;
class EpollPoller;

class EventLoop{
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    bool m_quit;
    std::unique_ptr<EpollPoller> m_poller;
    std::vector<Channel*> m_activeChannels;
};


/***************************************/
/* FILE: ./net/session/CMakeLists.txt */
/***************************************/

add_library(session STATIC
    Session.cpp SessionManager.cpp)

target_include_directories(session
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(session
    PUBLIC
        project_options
        log
        connection
)


/***************************************/
/* FILE: ./net/session/Session.cpp */
/***************************************/

#include "Session.h"

Session::Session(std::shared_ptr<TcpConnection> conn)
    : m_conn(std::move(conn)) {}

Session::~Session() = default;

std::shared_ptr<TcpConnection> Session::connection() const
{
    return m_conn;
}

void Session::setUserId(int id)
{
    m_userId = id;
}

int Session::userId() const
{
    return m_userId;
}

bool Session::isAuthenticated() const
{
    return m_userId > 0;
}

void Session::setPeerAddr(const sockaddr_in &addr)
{
    m_peerAddr = addr;
}

sockaddr_in Session::peerAddr() const
{
    return m_peerAddr;
}


/***************************************/
/* FILE: ./net/session/Session.h */
/***************************************/

#pragma once
#include <memory>
#include <netinet/in.h>
#include "TcpConnection.h"

class Session{
public:
    explicit Session(std::shared_ptr<TcpConnection> conn);
    ~Session();

    std::shared_ptr<TcpConnection> connection() const;

    void setUserId(int id);
    int userId() const;

    bool isAuthenticated() const;

    void setPeerAddr(const sockaddr_in &addr);
    sockaddr_in peerAddr() const;

private:
    std::shared_ptr<TcpConnection> m_conn;
    int m_userId{-1};
    sockaddr_in m_peerAddr{};
};


/***************************************/
/* FILE: ./net/session/SessionManager.cpp */
/***************************************/

#include "SessionManager.h"
#include "Logger.h"

std::unordered_map<int, std::shared_ptr<Session>> SessionManager::m_sessionsByFd;
std::unordered_map<int, std::vector<std::weak_ptr<Session>>> SessionManager::m_sessionsByUserId;
std::mutex SessionManager::m_mutex;

void SessionManager::addSession(int fd, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessionsByFd[fd] = session;
    LOG_INFO("Session added, fd={}", fd);
}

void SessionManager::removeSession(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessionsByFd.find(fd);
    if (it == m_sessionsByFd.end())
        return;

    auto session = it->second;
    m_sessionsByFd.erase(it);

    if (session->isAuthenticated()) {
        int userId = session->userId();
        auto uit = m_sessionsByUserId.find(userId);
        if (uit != m_sessionsByUserId.end()) {
            auto& vec = uit->second;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [&](const std::weak_ptr<Session>& wp) {
                        auto sp = wp.lock();
                        return !sp || sp == session;
                    }),
                vec.end()
            );

            if (vec.empty()) {
                m_sessionsByUserId.erase(uit);
            }
        }
    }

    LOG_INFO("Session removed, fd={}", fd);
}

std::shared_ptr<Session> SessionManager::getSession(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessionsByFd.find(fd);
    return it != m_sessionsByFd.end() ? it->second : nullptr;
}

void SessionManager::bindUser(int userId, const std::shared_ptr<Session> &session)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessionsByUserId[userId].push_back(session);
    LOG_INFO("Session bound to userId={}, fd={}", userId, session->connection()->fd());
}

std::vector<std::shared_ptr<Session>> SessionManager::getSessionByUserId(int userId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::shared_ptr<Session>> result;

    auto it = m_sessionsByUserId.find(userId);
    if (it == m_sessionsByUserId.end())
        return result;

    auto& vec = it->second;

    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [&](const std::weak_ptr<Session>& wp) {
                auto sp = wp.lock();
                if (sp) {
                    result.push_back(sp);
                    return false;
                }
                return true;
            }),
        vec.end()
    );

    return result;
}



/***************************************/
/* FILE: ./net/session/SessionManager.h */
/***************************************/

#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include "Session.h"

class SessionManager {
public:

    static void addSession(int fd, std::shared_ptr<Session> session);
    static void removeSession(int fd);
    static std::shared_ptr<Session> getSession(int fd);

    static void bindUser(int userId, const std::shared_ptr<Session>& session);
    static std::vector<std::shared_ptr<Session>> getSessionByUserId(int userId);

private:
    static std::unordered_map<int, std::shared_ptr<Session>> m_sessionsByFd;
    static std::unordered_map<int, std::vector<std::weak_ptr<Session>>> m_sessionsByUserId;
    static std::mutex m_mutex;
};



/***************************************/
/* FILE: ./protocol/CMakeLists.txt */
/***************************************/

add_subdirectory(codec)
add_subdirectory(protocol)


/***************************************/
/* FILE: ./protocol/codec/CMakeLists.txt */
/***************************************/

add_library(codec STATIC
    HttpCodec.cpp Codec.h WebSocketCodec.cpp)

target_include_directories(codec
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(codec
    PUBLIC
        project_options
        log
        buffer
        protocol
)


/***************************************/
/* FILE: ./protocol/codec/Codec.h */
/***************************************/

#pragma once
#include <vector>
#include <memory>
#include "Buffer.h"
#include "Message.h"

class Codec {
public:
    virtual ~Codec() = default;
    virtual std::vector<std::shared_ptr<Message>> decode(Buffer& buf) = 0;

    virtual void encode(const std::shared_ptr<Message>& msg, Buffer& buf) = 0;
};


/***************************************/
/* FILE: ./protocol/codec/HttpCodec.cpp */
/***************************************/

#include "HttpCodec.h"
#include "Buffer.h"
#include "Logger.h"
#include <sstream>
#include <algorithm>
#include <cstring>
#include <string>
#include <ctime>
#include <iomanip>
#include <map>

std::vector<std::shared_ptr<Message>> HttpCodec::decode(Buffer& buf) {
    std::vector<std::shared_ptr<Message>> messages;
    
    if (buf.readableBytes() == 0) {
        return messages;
    }
    
    std::string data = buf.retrieveUtf8String();
    if (data.empty()) {
        return messages;
    }
    
    // 查找请求结束位置（\r\n\r\n）
    size_t headerEnd = data.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // 可能请求不完整，将数据放回缓冲区
        buf.append(data);
        return messages;
    }
    
    size_t pos = 0;
    
    // 解析请求行
    size_t lineEnd = data.find("\r\n", pos);
    if (lineEnd == std::string::npos) {
        buf.append(data);
        return messages;
    }
    
    std::string requestLine = data.substr(pos, lineEnd - pos);
    
    HttpRequest::Method method;
    std::string path, version;
    
    if (!parseRequestLine(requestLine, method, path, version)) {
        LOG_ERROR("Failed to parse request line: {}", requestLine);
        buf.append(data);
        return messages;
    }
    
    auto req = std::make_shared<HttpRequest>();
    req->method = method;
    req->version = version;
    
    // 解析路径和查询参数
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        req->path = path.substr(0, queryPos);
        std::string queryStr = path.substr(queryPos + 1);
        parseQueryParams(queryStr, req->params);
    } else {
        req->path = path;
    }
    
    pos = lineEnd + 2; // 跳过 \r\n
    
    // 解析头部
    while (pos < headerEnd) {
        lineEnd = data.find("\r\n", pos);
        if (lineEnd == std::string::npos) {
            break;
        }
        
        std::string headerLine = data.substr(pos, lineEnd - pos);
        if (headerLine.empty()) {
            break;
        }
        
        std::string key, value;
        if (parseHeader(headerLine, key, value)) {
            req->headers[key] = value;
            
            // 解析Cookie
            if (key == "Cookie") {
                parseCookies(value, req->cookies);
            }
        }
        
        pos = lineEnd + 2;
    }
    
    // 解析请求体
    pos = headerEnd + 4; // 跳过 \r\n\r\n
    
    // 检查是否有Content-Length
    if (req->headers.find("Content-Length") != req->headers.end()) {
        try {
            size_t contentLength = std::stoul(req->headers["Content-Length"]);
            if (pos + contentLength <= data.size()) {
                req->body = data.substr(pos, contentLength);
                pos += contentLength;
            } else {
                // 数据不完整，放回缓冲区
                buf.append(data);
                return messages;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Invalid Content-Length: {}", e.what());
        }
    } else if (req->headers.find("Transfer-Encoding") != req->headers.end() &&
               req->headers["Transfer-Encoding"] == "chunked") {
        // TODO: 实现分块传输编码
        LOG_WARN("Chunked transfer encoding not yet supported");
    }
    
    // 添加请求到消息列表
    messages.push_back(req);
    
    // 检查是否还有更多请求（pipelining）
    if (pos < data.size()) {
        std::string remaining = data.substr(pos);
        buf.append(remaining);
        auto moreMessages = decode(buf);
        messages.insert(messages.end(), moreMessages.begin(), moreMessages.end());
    }
    
    return messages;
}

void HttpCodec::encode(const std::shared_ptr<Message>& msg, Buffer& buf) {
    if (msg->type() == MessageType::HTTP_REQUEST) {
        auto req = std::dynamic_pointer_cast<HttpRequest>(msg);
        if (!req) return;
        
        std::string raw;
        
        // 请求行
        switch (req->method) {
            case HttpRequest::Method::GET: raw += "GET "; break;
            case HttpRequest::Method::POST: raw += "POST "; break;
            case HttpRequest::Method::PUT: raw += "PUT "; break;
            case HttpRequest::Method::DELETE: raw += "DELETE "; break;
            case HttpRequest::Method::HEAD: raw += "HEAD "; break;
            case HttpRequest::Method::OPTIONS: raw += "OPTIONS "; break;
            case HttpRequest::Method::PATCH: raw += "PATCH "; break;
            default: raw += "GET "; break;
        }
        
        // 构建完整路径（包括查询参数）
        std::string fullPath = req->path;
        if (!req->params.empty()) {
            fullPath += "?";
            bool first = true;
            for (const auto& [key, value] : req->params) {
                if (!first) fullPath += "&";
                first = false;
                fullPath += key + "=" + value;
            }
        }
        
        raw += fullPath + " " + req->version + "\r\n";
        
        // 头部
        for (const auto& [key, value] : req->headers) {
            raw += key + ": " + value + "\r\n";
        }
        
        // 如果有Cookie，添加Cookie头
        if (!req->cookies.empty()) {
            std::string cookieHeader;
            bool first = true;
            for (const auto& [key, value] : req->cookies) {
                if (!first) cookieHeader += "; ";
                first = false;
                cookieHeader += key + "=" + value;
            }
            raw += "Cookie: " + cookieHeader + "\r\n";
        }
        
        raw += "\r\n";
        
        // 请求体
        if (!req->body.empty()) {
            raw += req->body;
        }
        
        buf.append(raw);
        
    } else if (msg->type() == MessageType::HTTP_RESPONSE) {
        auto resp = std::dynamic_pointer_cast<HttpResponse>(msg);
        if (!resp) return;
        
        std::string encoded = encodeResponse(resp);
        buf.append(encoded);
    }
}

std::string HttpCodec::encodeResponse(const std::shared_ptr<HttpResponse>& resp) {
    std::ostringstream oss;
    
    // 状态行
    oss << resp->version << " "
        << static_cast<int>(resp->statusCode) << " ";
    
    // 状态文本
    if (!resp->statusText.empty()) {
        oss << resp->statusText;
    } else {
        // 默认状态文本
        static const std::map<HttpResponse::StatusCode, std::string> defaultTexts = {
            {HttpResponse::StatusCode::OK, "OK"},
            {HttpResponse::StatusCode::CREATED, "Created"},
            {HttpResponse::StatusCode::NO_CONTENT, "No Content"},
            {HttpResponse::StatusCode::BAD_REQUEST, "Bad Request"},
            {HttpResponse::StatusCode::UNAUTHORIZED, "Unauthorized"},
            {HttpResponse::StatusCode::FORBIDDEN, "Forbidden"},
            {HttpResponse::StatusCode::NOT_FOUND, "Not Found"},
            {HttpResponse::StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
            {HttpResponse::StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
            {HttpResponse::StatusCode::SERVICE_UNAVAILABLE, "Service Unavailable"}
        };
        
        auto it = defaultTexts.find(resp->statusCode);
        if (it != defaultTexts.end()) {
            oss << it->second;
        } else {
            oss << "Unknown";
        }
    }
    oss << "\r\n";
    
    // 自动添加Date头
    time_t now = time(nullptr);
    char dateBuf[64];
    strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    oss << "Date: " << dateBuf << "\r\n";
    
    // 自动添加Server头
    oss << "Server: ChatServer/1.0\r\n";
    
    // 用户自定义头部
    for (const auto& [key, value] : resp->headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    // 自动添加Content-Length
    if (!resp->body.empty()) {
        oss << "Content-Length: " << resp->body.size() << "\r\n";
    }
    
    // Connection头
    bool keepAlive = true;
    auto connIt = resp->headers.find("Connection");
    if (connIt != resp->headers.end()) {
        keepAlive = (connIt->second == "keep-alive");
    }
    oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    
    oss << "\r\n";
    
    // 响应体
    if (!resp->body.empty()) {
        oss << resp->body;
    }
    
    return oss.str();
}

bool HttpCodec::parseRequestLine(const std::string& line,
                                HttpRequest::Method& method,
                                std::string& path,
                                std::string& version) {
    std::istringstream iss(line);
    std::string methodStr, pathStr, versionStr;
    
    if (!(iss >> methodStr >> pathStr >> versionStr)) {
        return false;
    }
    
    method = parseHttpMethod(methodStr);
    path = pathStr;
    version = parseHttpVersion(versionStr);
    
    return true;
}

bool HttpCodec::parseHeader(const std::string& line,
                           std::string& key,
                           std::string& value) {
    size_t colonPos = line.find(':');
    if (colonPos == std::string::npos) {
        return false;
    }
    
    key = line.substr(0, colonPos);
    
    // 去除key两端的空格
    size_t keyStart = key.find_first_not_of(" \t");
    size_t keyEnd = key.find_last_not_of(" \t");
    if (keyStart != std::string::npos && keyEnd != std::string::npos) {
        key = key.substr(keyStart, keyEnd - keyStart + 1);
    }
    
    // 获取value
    value = line.substr(colonPos + 1);
    
    // 去除value两端的空格
    size_t valueStart = value.find_first_not_of(" \t");
    size_t valueEnd = value.find_last_not_of(" \t");
    if (valueStart != std::string::npos && valueEnd != std::string::npos) {
        value = value.substr(valueStart, valueEnd - valueStart + 1);
    }
    
    // 如果value以\r结尾，去掉
    if (!value.empty() && value.back() == '\r') {
        value.pop_back();
    }
    
    return true;
}

std::shared_ptr<HttpRequest> HttpCodec::parseRequest(const std::string& raw) {
    auto req = std::make_shared<HttpRequest>();
    
    std::istringstream iss(raw);
    std::string line;
    
    // 解析请求行
    if (!std::getline(iss, line)) {
        return nullptr;
    }
    
    // 去除可能的\r
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    
    HttpRequest::Method method;
    std::string path, version;
    
    if (!parseRequestLine(line, method, path, version)) {
        return nullptr;
    }
    
    req->method = method;
    req->version = version;
    
    // 解析路径和查询参数
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos) {
        req->path = path.substr(0, queryPos);
        std::string queryStr = path.substr(queryPos + 1);
        parseQueryParams(queryStr, req->params);
    } else {
        req->path = path;
    }
    
    // 解析头部
    while (std::getline(iss, line) && !line.empty()) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (line.empty()) {
            break;
        }
        
        std::string key, value;
        if (parseHeader(line, key, value)) {
            req->headers[key] = value;
            
            // 解析Cookie
            if (key == "Cookie") {
                parseCookies(value, req->cookies);
            }
        }
    }
    
    // 解析请求体
    std::string body((std::istreambuf_iterator<char>(iss)),
                     std::istreambuf_iterator<char>());
    req->body = body;
    
    return req;
}

std::string HttpCodec::parseHttpVersion(const std::string& versionStr) {
    if (versionStr == "HTTP/1.0") {
        return "HTTP/1.0";
    } else if (versionStr == "HTTP/1.1") {
        return "HTTP/1.1";
    } else if (versionStr == "HTTP/2.0") {
        return "HTTP/2.0";
    } else {
        return versionStr;
    }
}

HttpRequest::Method HttpCodec::parseHttpMethod(const std::string& methodStr) {
    if (methodStr == "GET") {
        return HttpRequest::Method::GET;
    } else if (methodStr == "POST") {
        return HttpRequest::Method::POST;
    } else if (methodStr == "PUT") {
        return HttpRequest::Method::PUT;
    } else if (methodStr == "DELETE") {
        return HttpRequest::Method::DELETE;
    } else if (methodStr == "HEAD") {
        return HttpRequest::Method::HEAD;
    } else if (methodStr == "OPTIONS") {
        return HttpRequest::Method::OPTIONS;
    } else if (methodStr == "PATCH") {
        return HttpRequest::Method::PATCH;
    } else {
        return HttpRequest::Method::UNKNOWN;
    }
}

void HttpCodec::parseQueryParams(const std::string& queryStr,
                                std::unordered_map<std::string, std::string>& params) {
    std::istringstream iss(queryStr);
    std::string pair;
    
    while (std::getline(iss, pair, '&')) {
        size_t equalPos = pair.find('=');
        if (equalPos != std::string::npos) {
            std::string key = pair.substr(0, equalPos);
            std::string value = pair.substr(equalPos + 1);
            
            // URL解码（简单实现）
            // TODO: 实现完整的URL解码
            params[key] = value;
        } else {
            params[pair] = "";
        }
    }
}

void HttpCodec::parseCookies(const std::string& cookieStr,
                            std::unordered_map<std::string, std::string>& cookies) {
    std::istringstream iss(cookieStr);
    std::string cookie;
    
    while (std::getline(iss, cookie, ';')) {
        // 去除空格
        size_t start = cookie.find_first_not_of(' ');
        size_t end = cookie.find_last_not_of(' ');
        if (start != std::string::npos && end != std::string::npos) {
            cookie = cookie.substr(start, end - start + 1);
        }
        
        size_t equalPos = cookie.find('=');
        if (equalPos != std::string::npos) {
            std::string key = cookie.substr(0, equalPos);
            std::string value = cookie.substr(equalPos + 1);
            cookies[key] = value;
        }
    }
}


/***************************************/
/* FILE: ./protocol/codec/HttpCodec.h */
/***************************************/

#pragma once

#include "Codec.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <sstream>
#include <memory>
#include <vector>
#include <string>

class HttpCodec : public Codec {
public:
    // 构造函数和析构函数
    HttpCodec() = default;
    virtual ~HttpCodec() = default;
    
    // 解码HTTP请求
    std::vector<std::shared_ptr<Message>> decode(Buffer& buf) override;
    
    // 编码HTTP响应或请求
    void encode(const std::shared_ptr<Message>& msg, Buffer& buf) override;
    
    // 快速创建HTTP响应
    static std::string encodeResponse(const std::shared_ptr<HttpResponse>& resp);
    
    // 解析请求行
    static bool parseRequestLine(const std::string& line, 
                                 HttpRequest::Method& method,
                                 std::string& path,
                                 std::string& version);
    
    // 解析头部
    static bool parseHeader(const std::string& line,
                            std::string& key,
                            std::string& value);
    
    // 解析完整的HTTP请求
    static std::shared_ptr<HttpRequest> parseRequest(const std::string& raw);
    
private:
    // 解析HTTP版本
    static std::string parseHttpVersion(const std::string& versionStr);
    
    // 解析HTTP方法
    static HttpRequest::Method parseHttpMethod(const std::string& methodStr);
    
    // 解析查询参数（?key=value&...）
    static void parseQueryParams(const std::string& queryStr,
                                 std::unordered_map<std::string, std::string>& params);
    
    // 解析Cookie头
    static void parseCookies(const std::string& cookieStr,
                             std::unordered_map<std::string, std::string>& cookies);
};


/***************************************/
/* FILE: ./protocol/codec/WebSocketCodec.cpp */
/***************************************/

// FILE: ./protocol/codec/WebSocketCodec.cpp
#include "WebSocketCodec.h"
#include "WebSocketFrame.h"
#include "Logger.h"
#include <arpa/inet.h>
#include <random>
#include <cstring>

std::vector<std::shared_ptr<Message>> WebSocketCodec::decode(Buffer& buf) {
    std::vector<std::shared_ptr<Message>> frames;
    
    while (buf.readableBytes() >= 2) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(buf.peek());
        
        // 解析帧头
        bool fin = (data[0] & 0x80) != 0;
        uint8_t opcode = data[0] & 0x0F;
        bool masked = (data[1] & 0x80) != 0;
        uint64_t payloadLen = data[1] & 0x7F;
        
        size_t headerSize = 2;
        
        // 扩展长度
        if (payloadLen == 126) {
            if (buf.readableBytes() < 4) break;
            payloadLen = (data[2] << 8) | data[3];
            headerSize += 2;
        } else if (payloadLen == 127) {
            if (buf.readableBytes() < 10) break;
            payloadLen = 0;
            for (int i = 0; i < 8; i++) {
                payloadLen = (payloadLen << 8) | data[2 + i];
            }
            headerSize += 8;
        }
        
        // 掩码键
        uint8_t maskingKey[4] = {0};
        if (masked) {
            if (buf.readableBytes() < headerSize + 4) break;
            std::memcpy(maskingKey, data + headerSize, 4);
            headerSize += 4;
        }
        
        // 检查是否有完整的数据
        if (buf.readableBytes() < headerSize + payloadLen) {
            break;
        }
        
        // 创建帧
        auto frame = std::make_shared<WebSocketFrame>();
        frame->fin = fin;
        frame->opcode = static_cast<WebSocketFrame::Opcode>(opcode);
        frame->masked = masked;
        if (masked) {
            std::memcpy(frame->maskingKey, maskingKey, 4);
        }
        
        // 复制payload
        const uint8_t* payloadData = data + headerSize;
        frame->payload.resize(payloadLen);
        std::memcpy(frame->payload.data(), payloadData, payloadLen);
        
        // 如果掩码了，解码
        if (masked) {
            for (size_t i = 0; i < payloadLen; i++) {
                frame->payload[i] ^= maskingKey[i % 4];
            }
        }
        
        frames.push_back(frame);
        buf.retrieve(headerSize + payloadLen);
    }
    
    return frames;
}

void WebSocketCodec::encode(const std::shared_ptr<Message>& msg, Buffer& buf) {
    auto frame = std::dynamic_pointer_cast<WebSocketFrame>(msg);
    if (!frame) return;
    
    std::vector<uint8_t> header;
    
    // 第一个字节
    uint8_t b1 = 0;
    if (frame->fin) b1 |= 0x80;
    b1 |= static_cast<uint8_t>(frame->opcode);
    header.push_back(b1);
    
    // 第二个字节
    uint8_t b2 = frame->masked ? 0x80 : 0x00;
    size_t payloadLen = frame->payload.size();
    
    if (payloadLen <= 125) {
        b2 |= payloadLen;
        header.push_back(b2);
    } else if (payloadLen <= 65535) {
        b2 |= 126;
        header.push_back(b2);
        header.push_back((payloadLen >> 8) & 0xFF);
        header.push_back(payloadLen & 0xFF);
    } else {
        b2 |= 127;
        header.push_back(b2);
        for (int i = 7; i >= 0; i--) {
            header.push_back((payloadLen >> (i * 8)) & 0xFF);
        }
    }
    
    // 掩码键
    if (frame->masked) {
        header.insert(header.end(), 
                     std::begin(frame->maskingKey), 
                     std::end(frame->maskingKey));
    }
    
    // 写入缓冲区
    buf.append(reinterpret_cast<const char*>(header.data()), header.size());
    
    // 写入payload
    if (frame->masked) {
        // 掩码payload
        std::vector<uint8_t> maskedPayload = frame->payload;
        for (size_t i = 0; i < maskedPayload.size(); i++) {
            maskedPayload[i] ^= frame->maskingKey[i % 4];
        }
        buf.append(reinterpret_cast<const char*>(maskedPayload.data()), 
                   maskedPayload.size());
    } else {
        buf.append(reinterpret_cast<const char*>(frame->payload.data()), 
                   frame->payload.size());
    }
}


/***************************************/
/* FILE: ./protocol/codec/WebSocketCodec.h */
/***************************************/

// FILE: ./protocol/codec/WebSocketCodec.h
#pragma once
#include "Codec.h"
#include "WebSocketFrame.h"
#include <memory>
#include <vector>

class WebSocketCodec : public Codec {
public:
    std::vector<std::shared_ptr<Message>> decode(Buffer& buf) override;
    void encode(const std::shared_ptr<Message>& msg, Buffer& buf) override;
    
    // WebSocket握手响应
    static std::string createHandshakeResponse(const std::string& key);
    
    // 生成掩码键
    static void generateMaskingKey(uint8_t key[4]);
};


/***************************************/
/* FILE: ./protocol/protocol/CMakeLists.txt */
/***************************************/

add_library(protocol STATIC
    HttpRequest.cpp HttpResponse.cpp WebSocketFrame.cpp Message.h)

target_include_directories(protocol
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(protocol
    PUBLIC
        project_options
        log
        buffer
)


/***************************************/
/* FILE: ./protocol/protocol/HttpRequest.cpp */
/***************************************/

// FILE: ./protocol/protocol/HttpRequest.cpp
#include "HttpRequest.h"
#include <sstream>
#include <algorithm>
#include <cstring>

std::string HttpRequest::toString() const {
    std::ostringstream oss;
    
    // 请求行
    switch(method) {
        case Method::GET: oss << "GET "; break;
        case Method::POST: oss << "POST "; break;
        case Method::PUT: oss << "PUT "; break;
        case Method::DELETE: oss << "DELETE "; break;
        case Method::HEAD: oss << "HEAD "; break;
        case Method::OPTIONS: oss << "OPTIONS "; break;
        case Method::PATCH: oss << "PATCH "; break;
        default: oss << "UNKNOWN "; break;
    }
    
    oss << path << " " << version << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    oss << "\r\n";
    
    // Body
    if (!body.empty()) {
        oss << body;
    }
    
    return oss.str();
}

std::string HttpRequest::getCookie(const std::string& key) const {
    auto it = cookies.find(key);
    return it != cookies.end() ? it->second : "";
}

void HttpRequest::parsePathParams(const std::string& pattern) {
    // TODO: 实现参数化路径解析
    (void)pattern;  // 消除未使用参数警告
    // 简单实现：/user/{id} -> /user/123
    // 这里需要根据实际情况实现更复杂的匹配
    pathParams.clear();
}


/***************************************/
/* FILE: ./protocol/protocol/HttpRequest.h */
/***************************************/

// FILE: ./protocol/protocol/HttpRequest.h
#pragma once
#include "Message.h"
#include <string>
#include <unordered_map>
#include <vector>

class HttpRequest : public Message {
public:
    enum class Method {
        UNKNOWN,
        GET,
        POST,
        PUT,
        DELETE,
        HEAD,
        OPTIONS,
        PATCH
    };
    
    HttpRequest() { m_type = MessageType::HTTP_REQUEST; }
    
    MessageType type() const override { return MessageType::HTTP_REQUEST; }
    std::string toString() const override;
    
    bool keepAlive() const {
        auto it = headers.find("Connection");
        if (it != headers.end()) {
            return it->second == "keep-alive";
        }
        return version == "HTTP/1.1"; // HTTP/1.1 默认保持连接
    }
    
    // 获取查询参数
    std::string getParam(const std::string& key) const {
        auto it = params.find(key);
        return it != params.end() ? it->second : "";
    }
    
    // 获取Header
    std::string getHeader(const std::string& key) const {
        auto it = headers.find(key);
        return it != headers.end() ? it->second : "";
    }
    
    // 获取Cookie
    std::string getCookie(const std::string& key) const;
    
    // 解析路径参数（如 /user/123 -> {"id": "123"}）
    void parsePathParams(const std::string& pattern);
    
    Method method{Method::GET};
    std::string path;
    std::string version{"HTTP/1.1"};
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> params;
    std::unordered_map<std::string, std::string> pathParams;
    std::string body;
    
    // 解析后的Cookie
    std::unordered_map<std::string, std::string> cookies;
    
    // 客户端地址
    std::string clientIp;
    int clientPort{0};
};


/***************************************/
/* FILE: ./protocol/protocol/HttpResponse.cpp */
/***************************************/

// FILE: ./protocol/protocol/HttpResponse.cpp
#include "HttpResponse.h"
#include <sstream>
#include <map>
#include <ctime>
#include "Logger.h"

std::string HttpResponse::toString() const {
    std::ostringstream oss;
    
    // 状态行
    oss << version << " " << static_cast<int>(statusCode) << " ";
    
    // 状态文本
    if (!statusText.empty()) {
        oss << statusText;
    } else {
        // 默认状态文本
        static const std::map<StatusCode, std::string> defaultTexts = {
            {StatusCode::OK, "OK"},
            {StatusCode::CREATED, "Created"},
            {StatusCode::NO_CONTENT, "No Content"},
            {StatusCode::BAD_REQUEST, "Bad Request"},
            {StatusCode::UNAUTHORIZED, "Unauthorized"},
            {StatusCode::FORBIDDEN, "Forbidden"},
            {StatusCode::NOT_FOUND, "Not Found"},
            {StatusCode::METHOD_NOT_ALLOWED, "Method Not Allowed"},
            {StatusCode::INTERNAL_SERVER_ERROR, "Internal Server Error"},
            {StatusCode::SERVICE_UNAVAILABLE, "Service Unavailable"}
        };
        
        auto it = defaultTexts.find(statusCode);
        oss << (it != defaultTexts.end() ? it->second : "Unknown");
    }
    
    oss << "\r\n";
    
    // Headers
    for (const auto& [key, value] : headers) {
        oss << key << ": " << value << "\r\n";
    }
    
    // Date头
    time_t now = time(nullptr);
    char buf[64];
    strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));
    oss << "Date: " << buf << "\r\n";
    
    // Content-Length
    if (!body.empty() || !filePath.empty()) {
        oss << "Content-Length: " << body.size() << "\r\n";
    }
    
    // Connection
    bool keepAlive = headers.find("Connection") != headers.end() && 
                     headers.at("Connection") == "keep-alive";
    oss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    
    oss << "\r\n";
    
    // Body
    if (!body.empty()) {
        oss << body;
    }
    
    return oss.str();
}

std::shared_ptr<HttpResponse> HttpResponse::create(StatusCode code, 
                                                   const std::string& body,
                                                   const std::string& contentType) {
    auto resp = std::make_shared<HttpResponse>();
    resp->statusCode = code;
    resp->body = body;
    resp->setHeader("Content-Type", contentType);
    return resp;
}

std::shared_ptr<HttpResponse> HttpResponse::createJson(const std::string& json,
                                                       StatusCode code) {
    auto resp = create(code, json, "application/json; charset=utf-8");
    return resp;
}

std::shared_ptr<HttpResponse> HttpResponse::createHtml(const std::string& html,
                                                       StatusCode code) {
    auto resp = create(code, html, "text/html; charset=utf-8");
    return resp;
}

void HttpResponse::setCookie(const std::string& name,
                             const std::string& value,
                             const std::string& path,
                             int maxAge,
                             bool httpOnly) {
    std::ostringstream oss;
    oss << name << "=" << value << "; Path=" << path << "; Max-Age=" << maxAge;
    if (httpOnly) {
        oss << "; HttpOnly";
    }
    
    // 添加到Header
    auto it = headers.find("Set-Cookie");
    if (it == headers.end()) {
        headers["Set-Cookie"] = oss.str();
    } else {
        // 多个Cookie用换行分隔
        it->second += "\r\nSet-Cookie: " + oss.str();
    }
}


/***************************************/
/* FILE: ./protocol/protocol/HttpResponse.h */
/***************************************/

// FILE: ./protocol/protocol/HttpResponse.h
#pragma once
#include "Message.h"
#include <string>
#include <unordered_map>

class HttpResponse : public Message {
public:
    enum class StatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        METHOD_NOT_ALLOWED = 405,
        INTERNAL_SERVER_ERROR = 500,
        SERVICE_UNAVAILABLE = 503
    };
    
    HttpResponse() { m_type = MessageType::HTTP_RESPONSE; }
    
    MessageType type() const override { return MessageType::HTTP_RESPONSE; }
    std::string toString() const override;
    
    // 快捷方法
    static std::shared_ptr<HttpResponse> create(StatusCode code = StatusCode::OK,
                                                const std::string& body = "",
                                                const std::string& contentType = "text/plain");
    
    static std::shared_ptr<HttpResponse> createJson(const std::string& json,
                                                    StatusCode code = StatusCode::OK);
    
    static std::shared_ptr<HttpResponse> createHtml(const std::string& html,
                                                   StatusCode code = StatusCode::OK);
    
    // 设置Header
    void setHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
    
    // 设置Cookie
    void setCookie(const std::string& name,
                  const std::string& value,
                  const std::string& path = "/",
                  int maxAge = 86400,
                  bool httpOnly = true);
    
    StatusCode statusCode{StatusCode::OK};
    std::string statusText;
    std::string version{"HTTP/1.1"};
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    
    // 文件路径（用于静态文件服务）
    std::string filePath;
};


/***************************************/
/* FILE: ./protocol/protocol/Message.h */
/***************************************/

// FILE: ./protocol/protocol/Message.h
#pragma once
#include <string>
#include <memory>

enum class MessageType {
    UNKNOWN = 0,
    HTTP_REQUEST,
    HTTP_RESPONSE,
    WEBSOCKET_FRAME
};

class Message {
public:
    virtual ~Message() = default;
    virtual MessageType type() const = 0;
    virtual std::string toString() const = 0;
    
protected:
    MessageType m_type{MessageType::UNKNOWN};
};


/***************************************/
/* FILE: ./protocol/protocol/WebSocketFrame.cpp */
/***************************************/

// FILE: ./protocol/protocol/WebSocketFrame.cpp
#include "WebSocketFrame.h"
#include <sstream>
#include <random>
#include <cstring>
#include <arpa/inet.h>

std::string WebSocketFrame::toString() const {
    std::ostringstream oss;
    oss << "WebSocketFrame{"
        << "fin=" << fin
        << ", opcode=" << static_cast<int>(opcode)
        << ", masked=" << masked
        << ", payload_size=" << payload.size()
        << "}";
    return oss.str();
}

std::vector<uint8_t> WebSocketFrame::serialize() const {
    std::vector<uint8_t> result;
    
    // 第一个字节
    uint8_t b1 = 0;
    if (fin) b1 |= 0x80;
    b1 |= static_cast<uint8_t>(opcode);
    result.push_back(b1);
    
    // 第二个字节
    uint8_t b2 = masked ? 0x80 : 0x00;
    size_t payloadLen = payload.size();
    
    if (payloadLen <= 125) {
        b2 |= payloadLen;
        result.push_back(b2);
    } else if (payloadLen <= 65535) {
        b2 |= 126;
        result.push_back(b2);
        result.push_back((payloadLen >> 8) & 0xFF);
        result.push_back(payloadLen & 0xFF);
    } else {
        b2 |= 127;
        result.push_back(b2);
        for (int i = 7; i >= 0; i--) {
            result.push_back((payloadLen >> (i * 8)) & 0xFF);
        }
    }
    
    // 掩码键
    if (masked) {
        result.insert(result.end(), std::begin(maskingKey), std::end(maskingKey));
    }
    
    // payload
    if (masked) {
        // 掩码payload
        std::vector<uint8_t> maskedPayload = payload;
        for (size_t i = 0; i < maskedPayload.size(); i++) {
            maskedPayload[i] ^= maskingKey[i % 4];
        }
        result.insert(result.end(), maskedPayload.begin(), maskedPayload.end());
    } else {
        result.insert(result.end(), payload.begin(), payload.end());
    }
    
    return result;
}

std::shared_ptr<WebSocketFrame> WebSocketFrame::parse(const uint8_t* data, size_t length) {
    if (length < 2) return nullptr;
    
    auto frame = std::make_shared<WebSocketFrame>();
    
    // 解析帧头
    frame->fin = (data[0] & 0x80) != 0;
    frame->opcode = static_cast<Opcode>(data[0] & 0x0F);
    frame->masked = (data[1] & 0x80) != 0;
    uint64_t payloadLen = data[1] & 0x7F;
    
    size_t headerSize = 2;
    
    // 扩展长度
    if (payloadLen == 126) {
        if (length < 4) return nullptr;
        payloadLen = (data[2] << 8) | data[3];
        headerSize += 2;
    } else if (payloadLen == 127) {
        if (length < 10) return nullptr;
        payloadLen = 0;
        for (int i = 0; i < 8; i++) {
            payloadLen = (payloadLen << 8) | data[2 + i];
        }
        headerSize += 8;
    }
    
    // 掩码键
    if (frame->masked) {
        if (length < headerSize + 4) return nullptr;
        std::memcpy(frame->maskingKey, data + headerSize, 4);
        headerSize += 4;
    }
    
    // 检查是否有完整的数据
    if (length < headerSize + payloadLen) {
        return nullptr;
    }
    
    // 复制payload
    const uint8_t* payloadData = data + headerSize;
    frame->payload.resize(payloadLen);
    std::memcpy(frame->payload.data(), payloadData, payloadLen);
    
    // 如果掩码了，解码
    if (frame->masked) {
        for (size_t i = 0; i < payloadLen; i++) {
            frame->payload[i] ^= frame->maskingKey[i % 4];
        }
    }
    
    return frame;
}

std::string WebSocketFrame::getText() const {
    if (opcode != Opcode::TEXT) return "";
    return std::string(reinterpret_cast<const char*>(payload.data()), payload.size());
}

void WebSocketFrame::setText(const std::string& text) {
    opcode = Opcode::TEXT;
    payload.resize(text.size());
    std::memcpy(payload.data(), text.data(), text.size());
}


/***************************************/
/* FILE: ./protocol/protocol/WebSocketFrame.h */
/***************************************/

// FILE: ./protocol/protocol/WebSocketFrame.h
#pragma once
#include "Message.h"
#include <string>
#include <vector>
#include <cstdint>

class WebSocketFrame : public Message {
public:
    enum class Opcode {
        CONTINUATION = 0x0,
        TEXT = 0x1,
        BINARY = 0x2,
        CLOSE = 0x8,
        PING = 0x9,
        PONG = 0xA
    };
    
    WebSocketFrame() { m_type = MessageType::WEBSOCKET_FRAME; }
    
    MessageType type() const override { return MessageType::WEBSOCKET_FRAME; }
    std::string toString() const override;
    
    // 序列化为网络字节
    std::vector<uint8_t> serialize() const;
    
    // 解析网络字节
    static std::shared_ptr<WebSocketFrame> parse(const uint8_t* data, size_t length);
    
    bool fin{true};
    Opcode opcode{Opcode::TEXT};
    bool masked{false};
    uint8_t maskingKey[4]{0};
    std::vector<uint8_t> payload;
    
    // 对于文本帧，方便获取字符串
    std::string getText() const;
    void setText(const std::string& text);
};


/***************************************/
/* FILE: ./service/CMakeLists.txt */
/***************************************/

add_subdirectory(HttpService)


/***************************************/
/* FILE: ./service/HttpService/CMakeLists.txt */
/***************************************/

add_library(httpservice STATIC
    HttpService.cpp)

target_include_directories(httpservice
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(httpservice
    PUBLIC
        project_options
        log
        codec
        connection
        protocol
        session
        thread_pool
        handler
        dispatcher
)


/***************************************/
/* FILE: ./service/HttpService/HttpService.cpp */
/***************************************/

// FILE: ./service/HttpService/HttpService.cpp
#include "HttpService.h"
#include "Logger.h"
#include "Thread_pool.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

HttpService::HttpService(const std::string& staticPath)
    : m_codec(),
      m_staticPath(staticPath) {
    LOG_INFO("HttpService created with static path: {}", staticPath);
    
    // 注册默认处理器
    registerDefaultHandlers();
}

void HttpService::registerDefaultHandlers() {
    // 静态文件服务
    Dispatcher::instance().registerHttpHandler(
        "static",
        {"/*"},
        {HttpRequest::Method::GET},
        [this](const std::shared_ptr<HttpRequest>& req,
               const std::shared_ptr<Session>& session) {
            return handleStaticFile(req);
        }
    );
    
    // 默认404处理器
    Dispatcher::instance().setDefault404Handler(
        [](const std::shared_ptr<HttpRequest>& req,
           const std::shared_ptr<Session>& session) {
            std::stringstream ss;
            ss << "<html><head><title>404 Not Found</title></head>"
               << "<body><h1>404 Not Found</h1>"
               << "<p>The requested URL " << req->path 
               << " was not found on this server.</p>"
               << "</body></html>";
            
            auto resp = HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                            ss.str(), "text/html");
            return resp;
        }
    );
}

void HttpService::onConnection(const TcpConnection::Ptr& conn) {
    LOG_INFO("HttpService new connection fd={}", conn->fd());

    conn->setReadCallback(
        [self = shared_from_this()](const TcpConnection::Ptr& c, Buffer& buf) {
            self->onMessage(c, buf);
        }
    );

    conn->setCloseCallback(
        [self = shared_from_this()](const TcpConnection::Ptr& c) {
            self->onClose(c);
        }
    );
}

void HttpService::onMessage(const TcpConnection::Ptr& conn, Buffer& buffer) {
    // 异步处理请求
    ThreadPool::detach_task([this, conn, buffer = std::move(buffer)]() mutable {
        processRequest(conn, buffer);
    });
}

void HttpService::processRequest(const TcpConnection::Ptr& conn, Buffer& buffer) {
    try {
        auto requests = m_codec.decode(buffer);
        
        for (auto& msg : requests) {
            if (msg->type() == MessageType::HTTP_REQUEST) {
                auto req = std::dynamic_pointer_cast<HttpRequest>(msg);
                if (!req) continue;
                
                // 获取会话
                auto session = SessionManager::getSession(conn->fd());
                if (!session) {
                    LOG_ERROR("No session found for fd={}", conn->fd());
                    continue;
                }
                
                // 设置客户端地址
                sockaddr_in addr = session->peerAddr();
                req->clientIp = inet_ntoa(addr.sin_addr);
                req->clientPort = ntohs(addr.sin_port);
                
                // 分发请求
                auto resp = Dispatcher::instance().dispatchHttp(req, session);
                if (!resp) {
                    resp = HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                               "Internal Server Error");
                }
                
                // 编码响应
                Buffer outBuf;
                m_codec.encode(resp, outBuf);
                
                // 发送响应（需要在IO线程中执行）
                conn->send(outBuf.retrieveAllAsString());
                
                // 检查是否保持连接
                if (!req->keepAlive()) {
                    conn->shutdown();
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing request: {}", e.what());
        
        // 发送错误响应
        auto resp = HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                        "Internal Server Error");
        Buffer outBuf;
        m_codec.encode(resp, outBuf);
        conn->send(outBuf.retrieveAllAsString());
    }
}

void HttpService::onClose(const TcpConnection::Ptr& conn) {
    LOG_INFO("HttpService connection closed fd={}", conn->fd());
}

std::shared_ptr<HttpResponse> HttpService::handleStaticFile(
    const std::shared_ptr<HttpRequest>& req) {
    
    if (m_staticPath.empty()) {
        return HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                   "Static file service not configured");
    }
    
    // 安全验证：防止路径遍历攻击
    std::string requestPath = req->path;
    if (requestPath.find("..") != std::string::npos) {
        return HttpResponse::create(HttpResponse::StatusCode::FORBIDDEN,
                                   "Forbidden");
    }
    
    // 构建文件路径
    std::string filePath = m_staticPath + requestPath;
    if (requestPath == "/") {
        filePath += "index.html";
    }
    
    // 检查文件是否存在
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
        return HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                   "File not found");
    }
    
    
    // 读取文件
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                   "Cannot read file");
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // 根据扩展名设置Content-Type
    std::string contentType = "application/octet-stream";
    std::string ext = fs::path(filePath).extension().string();
    
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".txt", "text/plain"},
        {".pdf", "application/pdf"}
    };
    
    auto it = mimeTypes.find(ext);
    if (it != mimeTypes.end()) {
        contentType = it->second;
    }
    
    auto resp = HttpResponse::create(HttpResponse::StatusCode::OK, content, contentType);
    resp->setHeader("Cache-Control", "public, max-age=3600");
    
    return resp;
}


/***************************************/
/* FILE: ./service/HttpService/HttpService.h */
/***************************************/

// FILE: ./service/HttpService/HttpService.h
#pragma once
#include <memory>
#include "TcpConnection.h"
#include "HttpCodec.h"
#include "Dispatcher.h"
#include "SessionManager.h"

class HttpService : public std::enable_shared_from_this<HttpService> {
public:
    using Ptr = std::shared_ptr<HttpService>;

    HttpService(const std::string& staticPath = "");
    ~HttpService() = default;

    void onConnection(const TcpConnection::Ptr& conn);
    void onMessage(const TcpConnection::Ptr& conn, Buffer& buffer);
    void onClose(const TcpConnection::Ptr& conn);
    
    // 注册自定义路由
    template<typename Func>
    void registerRoute(const std::string& name,
                       const std::vector<std::string>& patterns,
                       const std::vector<HttpRequest::Method>& methods,
                       Func handler) {
        Dispatcher::instance().registerHttpHandler(name, patterns, methods, handler);
    }
    
    // 快捷方法：注册GET路由
    template<typename Func>
    void get(const std::string& pattern, Func handler) {
        registerRoute("GET_" + pattern, {pattern}, 
                      {HttpRequest::Method::GET}, handler);
    }
    
    // 快捷方法：注册POST路由
    template<typename Func>
    void post(const std::string& pattern, Func handler) {
        registerRoute("POST_" + pattern, {pattern}, 
                      {HttpRequest::Method::POST}, handler);
    }

private:
    void processRequest(const TcpConnection::Ptr& conn, Buffer& buffer);
    void registerDefaultHandlers();
    std::shared_ptr<HttpResponse> handleStaticFile(const std::shared_ptr<HttpRequest>& req);

private:
    HttpCodec m_codec;
    std::string m_staticPath;
};


