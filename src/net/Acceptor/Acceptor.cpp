// Acceptor/Acceptor.cpp
#include "Acceptor.h"
#include "Logger.h"
#include <iostream>

Acceptor::Acceptor(std::shared_ptr<EventLoop> loop, uint16_t port,
                   std::shared_ptr<SessionManager> sessionManager)
    : m_loop(loop),
      m_acceptor(loop->getIOContext(), boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      m_sessionManager(sessionManager) {
    LOG_INFO("[Acceptor] Created on port {}", port);
}

void Acceptor::startAccept() {
    LOG_INFO("[Acceptor] startAccept called");
    doAccept();
}

void Acceptor::stop() {
    LOG_INFO("[Acceptor] stop called");
    boost::system::error_code ec;
    m_acceptor.close(ec);
    if(ec) {
        std::cerr << "Acceptor close error: " << ec.message() << std::endl;
    }
}

void Acceptor::doAccept() {
    LOG_DEBUG("[Acceptor] doAccept called");

    m_acceptor.async_accept(
        [this](boost::system::error_code ec,
                     boost::asio::ip::tcp::socket socket) {

            if (ec) {
                LOG_ERROR("[Acceptor] accept error: {}", ec.message());
                return;
            }

            LOG_INFO("[Acceptor] new connection from {}",
                     socket.remote_endpoint().address().to_string());

            auto conn = std::make_shared<HttpConnection>(std::move(socket));
            conn->start();
            doAccept();
        }
    );
}