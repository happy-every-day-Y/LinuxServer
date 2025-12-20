#include "Acceptor.h"
#include "Logger.h"
#include <unistd.h>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int setNonBlocking(int fd){
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return 0;
}

Acceptor::Acceptor(EventLoop *loop, const sockaddr_in &listenAddr)
    : m_loop(loop), m_listenFd(socket(AF_INET, SOCK_STREAM, 0))
{
    if(m_listenFd < 0){
        LOG_ERROR("Acceptor create socket failed: {}", strerror(errno));
        return;
    }

    setNonBlocking(m_listenFd);
    
    int opt = 1;
    setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    if(bind(m_listenFd, (sockaddr*)&listenAddr, sizeof(listenAddr)) < 0){
        LOG_ERROR("Acceptor bind failed: {}", strerror(errno));
        close(m_listenFd);
        return;
    }

    m_acceptChannel = std::make_unique<Channel>(m_loop, m_listenFd);
    m_acceptChannel->setReadCallback([this]{ handleRead(); });
}

Acceptor::~Acceptor()
{
    close(m_listenFd);
}

void Acceptor::listen()
{
    ::listen(m_listenFd, SOMAXCONN);
    m_acceptChannel->enableReading();
    LOG_INFO("Acceptor listening fd={}", m_listenFd);
}

void Acceptor::handleRead()
{
    sockaddr_in peerAddr{};
    socklen_t addrLen = sizeof(peerAddr);

    int connfd = ::accept(m_listenFd, (sockaddr*)&peerAddr, &addrLen);
    if(connfd >= 0) {
        setNonBlocking(connfd);
        LOG_INFO("Accepted new connection fd={} from {}:{}", 
                 connfd, inet_ntoa(peerAddr.sin_addr), ntohs(peerAddr.sin_port));

        if(m_newConnectionCallback) {
            m_newConnectionCallback(connfd, peerAddr);
        }
    } else {
        LOG_ERROR("accept error: {}", strerror(errno));
    }
}
