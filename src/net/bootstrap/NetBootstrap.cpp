#include "NetBootstrap.h"
#include "Logger.h"

NetBootstrap::NetBootstrap(EventLoop *loop, const sockaddr_in &listenAddr)
    : m_loop(loop), m_acceptor(m_loop, listenAddr)
{
    m_acceptor.setNewConnectionCallback(
        [this](int fd, const sockaddr_in& addr){
            handleNewConnection(fd, addr);
        }
    );
}

void NetBootstrap::setReadCallback(ReadCallback cb)
{
    m_readCallback = std::move(cb);
}

void NetBootstrap::start()
{
    m_acceptor.listen();
    m_loop->loop();
}

void NetBootstrap::handleNewConnection(int fd, const sockaddr_in &addr)
{
    auto conn = std::make_shared<TcpConnection>(m_loop, fd);

    auto session = std::make_shared<Session>(conn);
    session->setPeerAddr(addr);
    SessionManager::addSession(fd, session);

    m_connections[fd] = conn;

    LOG_INFO("Session created, fd={}", fd);

    conn->setReadCallback(m_readCallback);
    conn->setCloseCallback([this](const std::shared_ptr<TcpConnection>& c){
        int fd = c->fd();

        LOG_INFO("Connection fd={} closed, removing from map", c->fd());

        SessionManager::removeSession(fd);
        m_connections.erase(fd);
    });

    LOG_INFO("New connection fd={} from {}:{}", fd,
             inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}
