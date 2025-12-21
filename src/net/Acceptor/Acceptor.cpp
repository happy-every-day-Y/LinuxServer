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

void Acceptor::doAccept(){
    auto self = shared_from_this();
    m_acceptor.async_accept(
        [this, self](boost::system::error_code ec, boost::asio::ip::tcp::socket socket){
            if (!ec) {
                auto tcp = std::make_shared<TcpConnection>(std::move(socket));
                auto httpConn = std::make_shared<HttpConnection>(tcp);
                httpConn->start();
                if (m_sessionManager){
                    auto session = m_sessionManager->createSession();
                    httpConn->bindSession(m_sessionManager->createSession());
                }
                LOG_INFO("New connection from {}", tcp->remoteAddr());
            }else { std::cerr << "Accept error: " << ec.message() << std::endl; }
            doAccept();
        }
    );
}