// NetBootstrap.cpp
#include "NetBootstrap.h"
#include "Logger.h"
#include <thread>

NetBootstrap::NetBootstrap() = default;

NetBootstrap::~NetBootstrap() {
    stop();
}

void NetBootstrap::start(uint16_t port) {
    // 1. 创建 EventLoop
    m_loop = std::make_shared<EventLoop>();

    // 2. 创建 SessionManager
    m_sessionManager = std::make_shared<SessionManager>();

    // 3. 创建 Acceptor
    m_acceptor = std::make_shared<Acceptor>(m_loop, port, m_sessionManager);

    // 4. 启动监听
    m_acceptor->startAccept();
    LOG_INFO("Server started at port {}", port);

    // 5. 启动 IO 循环（阻塞调用）
    //    如果想非阻塞，可换成 std::thread 启动
    m_loop->run();
}

void NetBootstrap::stop() {
    LOG_INFO("Stopping server...");

    // 1. 停止 Acceptor
    if (m_acceptor) {
        m_acceptor->stop();
        m_acceptor.reset();
    }

    // 2. 关闭所有 Session（并通过 Session detach 所有连接）
    if (m_sessionManager) {
        m_sessionManager->removeAllSessions();
        m_sessionManager.reset();
    }

    // 3. 停止 EventLoop
    if (m_loop) {
        m_loop->stop();
        m_loop.reset();
    }

    LOG_INFO("Server stopped");
}
