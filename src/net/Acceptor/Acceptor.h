// Acceptor/Acceptor.h
#pragma once
#include <boost/asio.hpp>
#include <memory>
#include "EventLoop.h"
#include "Session.h"
#include "SessionManager.h"
#include "HttpConnection.h"
#include "WebSocketConnection.h"

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
