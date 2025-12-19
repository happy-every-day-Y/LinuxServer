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