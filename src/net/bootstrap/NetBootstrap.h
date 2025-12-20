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