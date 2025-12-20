#include "Session.h"

Session::Session(std::shared_ptr<TcpConnection> conn)
    : m_conn(std::move(conn)) {}

Session::~Session() = default;

std::shared_ptr<TcpConnection> Session::connection() const
{
    return m_conn;
}

void Session::setUserId(int id)
{
    m_userId = id;
}

int Session::userId() const
{
    return m_userId;
}

bool Session::isAuthenticated() const
{
    return m_userId > 0;
}

void Session::setPeerAddr(const sockaddr_in &addr)
{
    m_peerAddr = addr;
}

sockaddr_in Session::peerAddr() const
{
    return m_peerAddr;
}