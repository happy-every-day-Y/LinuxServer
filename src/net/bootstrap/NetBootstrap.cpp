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

void NetBootstrap::setMessageCallback(MessageCallback cb)
{
    m_messageCallback = std::move(cb);
}

void NetBootstrap::start()
{
    m_acceptor.listen();
    m_loop->loop();
}

void NetBootstrap::handleNewConnection(int fd, const sockaddr_in &addr)
{
    auto conn = std::make_shared<TcpConnection>(m_loop, fd);

    m_connections[fd] = conn;

    conn->setMessageCallback(m_messageCallback);
    conn->setCloseCallback([this](const std::shared_ptr<TcpConnection>& c){
        LOG_INFO("Connection fd={} closed, removing from map", c->fd());
        m_connections.erase(c->fd());
    });

    LOG_INFO("New connection fd={} from {}:{}", fd,
             inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
}
