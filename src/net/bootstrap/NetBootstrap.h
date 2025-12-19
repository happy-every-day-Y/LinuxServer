#pragma once
#include "EventLoop.h"
#include "Acceptor.h"
#include "TcpConnection.h"
#include "Buffer.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory>
#include <unordered_map>

class NetBootstrap{
public:
    using MessageCallback = TcpConnection::MessageCallback;

    NetBootstrap(EventLoop* loop, const sockaddr_in& listenAddr);
    void setMessageCallback(MessageCallback cb);
    void start();

private:
    EventLoop* m_loop;
    Acceptor m_acceptor;
    MessageCallback m_messageCallback;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> m_connections;

    void handleNewConnection(int fd, const sockaddr_in& addr);
};