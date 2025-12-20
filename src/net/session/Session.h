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