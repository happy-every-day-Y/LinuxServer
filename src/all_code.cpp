// ===== MERGED SOURCE FILE =====
// Generated on Sun Dec 21 11:43:44 CST 2025

/***************************************/
/* FILE: ./CMakeLists.txt */
/***************************************/

add_subdirectory(base)
add_subdirectory(config)
add_subdirectory(net)


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
/* FILE: ./net/Acceptor/Acceptor.cpp */
/***************************************/

// Acceptor/Acceptor.cpp
#include "Acceptor.h"
#include "Logger.h"
#include <iostream>

Acceptor::Acceptor(std::shared_ptr<EventLoop> loop, uint16_t port,
                   std::shared_ptr<SessionManager> sessionManager)
    : m_loop(loop),
      m_acceptor(loop->getIOContext(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      m_sessionManager(sessionManager) {}

void Acceptor::startAccept() {
    doAccept();
}

void Acceptor::stop() {
    boost::system::error_code ec;
    m_acceptor.close(ec);
    if(ec) {
        std::cerr << "Acceptor close error: " << ec.message() << std::endl;
    }
}

void Acceptor::doAccept() {
    auto self = shared_from_this();
    m_acceptor.async_accept(
        [this, self](boost::system::error_code ec, boost::asio::ip::tcp::socket socket){
            if(!ec) {
                // 创建 TcpConnection
                auto conn = std::make_shared<TcpConnection>(std::move(socket));

                // 创建 Session 并绑定
                if(m_sessionManager) {
                    auto session = m_sessionManager->createSession(conn);
                    conn->setSession(session);
                }

                LOG_INFO("New connection from {}", conn->getRemoteAddr());

                conn->asyncRead(); // 启动异步读
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            // 持续接收
            doAccept();
        }
    );
}



/***************************************/
/* FILE: ./net/Acceptor/Acceptor.h */
/***************************************/

// Acceptor/Acceptor.h
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include "TcpConnection.h"
#include "EventLoop.h"
#include "Session.h"
#include "SessionManager.h"

class Acceptor : public std::enable_shared_from_this<Acceptor> {
public:
    Acceptor(std::shared_ptr<EventLoop> loop, uint16_t port,
             std::shared_ptr<SessionManager> sessionManager);

    void startAccept();
    void stop();

private:
    void doAccept();

private:
    std::shared_ptr<EventLoop> m_loop;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::shared_ptr<SessionManager> m_sessionManager;
};



/***************************************/
/* FILE: ./net/Acceptor/CMakeLists.txt */
/***************************************/

add_library(acceptor STATIC
    Acceptor.cpp)

target_include_directories(acceptor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(acceptor
    PUBLIC
        project_options
        log
        tcpconnection
        eventloop
)


/***************************************/
/* FILE: ./net/CMakeLists.txt */
/***************************************/

add_subdirectory(Acceptor)
add_subdirectory(EventLoop)
add_subdirectory(NetBootstrap)
add_subdirectory(Session)
add_subdirectory(TcpConnection)


/***************************************/
/* FILE: ./net/EventLoop/CMakeLists.txt */
/***************************************/

add_library(eventloop STATIC
    EventLoop.cpp)

target_include_directories(eventloop
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(eventloop
    PUBLIC
        project_options
        log
)


/***************************************/
/* FILE: ./net/EventLoop/EventLoop.cpp */
/***************************************/

// EventLoop.cpp
#include "EventLoop.h"

EventLoop::EventLoop()
    : m_ioContext(),
      m_workGuard(boost::asio::make_work_guard(m_ioContext)) {}

EventLoop::~EventLoop() {
    stop();
    if(m_thread.joinable()) m_thread.join();
}

void EventLoop::run() {
    m_thread = std::thread([this]{
        m_ioContext.run();
    });
}

void EventLoop::stop() {
    m_ioContext.stop();
}

void EventLoop::post(std::function<void()> cb) {
    boost::asio::post(m_ioContext, std::move(cb));
}

boost::asio::io_context &EventLoop::getIOContext()
{
     return m_ioContext;
}



/***************************************/
/* FILE: ./net/EventLoop/EventLoop.h */
/***************************************/

// EventLoop.h
#pragma once
#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <functional>

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void run();                         // 启动事件循环
    void stop();                        // 停止事件循环
    void post(std::function<void()> cb); // 投递任务到 io_context
    boost::asio::io_context& getIOContext();

private:
    boost::asio::io_context m_ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
    std::thread m_thread;
};



/***************************************/
/* FILE: ./net/NetBootstrap/CMakeLists.txt */
/***************************************/

add_library(netbootstrap STATIC
    NetBootstrap.cpp)

target_include_directories(netbootstrap
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(netbootstrap
    PUBLIC
        project_options
        log
        acceptor
        eventloop
        session
)


/***************************************/
/* FILE: ./net/NetBootstrap/NetBootstrap.cpp */
/***************************************/

#include "NetBootstrap.h"
#include <iostream>

NetBootstrap::NetBootstrap() = default;

NetBootstrap::~NetBootstrap() {
    stop();
}

void NetBootstrap::start(uint16_t port) {
    // 1. 创建 EventLoop
    m_loop = std::make_shared<EventLoop>();
    m_loop->run();

    // 2. 创建 SessionManager
    m_sessionManager = std::make_shared<SessionManager>();

    // 3. 创建 Acceptor
    m_acceptor = std::make_shared<Acceptor>(
        m_loop,
        port,
        m_sessionManager
    );

    // 4. 启动监听
    m_acceptor->startAccept();

    std::cout << "[NetBootstrap] Server started at port " << port << std::endl;
}

void NetBootstrap::stop() {
    if (m_acceptor) {
        m_acceptor->stop();
        m_acceptor.reset();
    }

    if (m_loop) {
        m_loop->stop();
        m_loop.reset();
    }

    m_sessionManager.reset();

    std::cout << "[NetBootstrap] Server stopped" << std::endl;
}



/***************************************/
/* FILE: ./net/NetBootstrap/NetBootstrap.h */
/***************************************/

#pragma once

#include <memory>
#include <cstdint>

#include "EventLoop.h"
#include "Acceptor.h"
#include "SessionManager.h"

class NetBootstrap {
public:
    NetBootstrap();
    ~NetBootstrap();

    void start(uint16_t port);
    void stop();

private:
    std::shared_ptr<EventLoop> m_loop;
    std::shared_ptr<SessionManager> m_sessionManager;
    std::shared_ptr<Acceptor> m_acceptor;
};



/***************************************/
/* FILE: ./net/Session/CMakeLists.txt */
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
        tcpconnection
)


/***************************************/
/* FILE: ./net/Session/Session.cpp */
/***************************************/

#include "Session.h"

Session::Session(uint64_t id)
    : m_id(id) {}

uint64_t Session::getId() const {
    return m_id;
}

void Session::setData(const std::string& key, const std::any& value) {
    m_data[key] = value;
}

std::any Session::getData(const std::string& key) const {
    auto it = m_data.find(key);
    if(it != m_data.end()) {
        return it->second;
    }
    return {};
}

bool Session::hasData(const std::string& key) const {
    return m_data.find(key) != m_data.end();
}



/***************************************/
/* FILE: ./net/Session/Session.h */
/***************************************/

#pragma once
#include <cstdint>
#include <unordered_map>
#include <any>
#include <memory>
#include <string>

class Session {
public:
    using Ptr = std::shared_ptr<Session>;

    Session(uint64_t id);

    uint64_t getId() const;

    void setData(const std::string& key, const std::any& value);
    std::any getData(const std::string& key) const;
    bool hasData(const std::string& key) const;

private:
    uint64_t m_id;
    std::unordered_map<std::string, std::any> m_data;
};



/***************************************/
/* FILE: ./net/Session/SessionManager.cpp */
/***************************************/

#include "SessionManager.h"

SessionManager::SessionManager()
    : m_nextSessionId(1) {}

std::shared_ptr<Session> SessionManager::createSession(std::shared_ptr<TcpConnection> conn) {
    std::lock_guard<std::mutex> lock(m_mutex);
    uint64_t id = m_nextSessionId++;
    auto session = std::make_shared<Session>(id);
    m_sessions[id] = session;
    return session;
}

void SessionManager::removeSession(uint64_t sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(sessionId);
}

std::shared_ptr<Session> SessionManager::getSession(uint64_t sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if(it != m_sessions.end()) {
        return it->second;
    }
    return nullptr;
}



/***************************************/
/* FILE: ./net/Session/SessionManager.h */
/***************************************/

#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include "Session.h"
#include "TcpConnection.h"

class SessionManager {
public:
    SessionManager();

    std::shared_ptr<Session> createSession(std::shared_ptr<TcpConnection> conn);
    void removeSession(uint64_t sessionId);
    std::shared_ptr<Session> getSession(uint64_t sessionId);

private:
    uint64_t m_nextSessionId;
    std::unordered_map<uint64_t, std::shared_ptr<Session>> m_sessions;
    std::mutex m_mutex;
};



/***************************************/
/* FILE: ./net/TcpConnection/CMakeLists.txt */
/***************************************/

add_library(tcpconnection STATIC
    TcpConnection.cpp)

target_include_directories(tcpconnection
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tcpconnection
    PUBLIC
        project_options
        log
        session
)


/***************************************/
/* FILE: ./net/TcpConnection/TcpConnection.cpp */
/***************************************/

// TcpConnection.cpp
#include "TcpConnection.h"
#include "Logger.h"
#include <iostream>

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)), m_session(nullptr) {}

void TcpConnection::asyncRead() {
    auto self = shared_from_this();
    m_socket.async_read_some(
        m_buffer.prepare(4096),
        [this, self](boost::system::error_code ec, std::size_t bytesTransferred){
            onRead(ec, bytesTransferred);
        }
    );
}

void TcpConnection::onRead(boost::system::error_code ec, std::size_t bytesTransferred) {
    if(ec) {
        std::cerr << "Read error: " << ec.message() << std::endl;
        return;
    }

    m_buffer.commit(bytesTransferred);
    std::string data{(const char*)m_buffer.data().data(), m_buffer.size()};
    m_buffer.consume(m_buffer.size());

    // TODO: 把 data 发给 Dispatcher 或上层业务处理
    if(m_session) {
        // 可以通过 Session 存储状态或路由消息
    }

    LOG_INFO("Received data: {}", data);

    // 持续读取
    asyncRead();
}

void TcpConnection::asyncWrite(const std::string& data) {
    auto self = shared_from_this();
    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(data),
        [this, self](boost::system::error_code ec, std::size_t bytesTransferred){
            onWrite(ec, bytesTransferred);
        }
    );
}

void TcpConnection::onWrite(boost::system::error_code ec, std::size_t /*bytesTransferred*/) {
    if(ec) {
        std::cerr << "Write error: " << ec.message() << std::endl;
    }
}

std::string TcpConnection::getRemoteAddr() const {
    try {
        return m_socket.remote_endpoint().address().to_string();
    } catch(...) {
        return "unknown";
    }
}

void TcpConnection::setSession(std::shared_ptr<Session> session) {
    m_session = session;
}

std::shared_ptr<Session> TcpConnection::getSession() const {
    return m_session;
}



/***************************************/
/* FILE: ./net/TcpConnection/TcpConnection.h */
/***************************************/

// TcpConnection.h
#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <memory>
#include <functional>
#include <string>
#include "Session.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using Ptr = std::shared_ptr<TcpConnection>;

    TcpConnection(boost::asio::ip::tcp::socket socket);

    void asyncRead();
    void asyncWrite(const std::string& data);

    std::string getRemoteAddr() const;

    void setSession(std::shared_ptr<Session> session);
    std::shared_ptr<Session> getSession() const;

private:
    void onRead(boost::system::error_code ec, std::size_t bytesTransferred);
    void onWrite(boost::system::error_code ec, std::size_t bytesTransferred);

private:
    boost::asio::ip::tcp::socket m_socket;
    boost::beast::flat_buffer m_buffer;
    std::shared_ptr<Session> m_session;
};



