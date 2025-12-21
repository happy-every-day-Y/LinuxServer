// NetBootstrap.h
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

    // 启动服务器，监听指定端口
    void start(uint16_t port);

    // 停止服务器，关闭所有连接和 Session
    void stop();

private:
    std::shared_ptr<EventLoop> m_loop;
    std::shared_ptr<SessionManager> m_sessionManager;
    std::shared_ptr<Acceptor> m_acceptor;
};
