#include "TcpConnection.h"
#include "Logger.h"

#include <unistd.h>
#include <errno.h>

TcpConnection::TcpConnection(EventLoop* loop, int fd)
    : m_loop(loop),
      m_channel(std::make_unique<Channel>(loop, fd)),
      m_inputBuffer(),
      m_outputBuffer(),
      m_state(Connecting)
{
    LOG_INFO("TcpConnection created fd={}", fd);

    m_channel->setReadCallback([this]() { handleRead(); });
    m_channel->setWriteCallback([this]() { handleWrite(); });
    m_channel->enableReading();

    m_state = Connected;
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection destroyed fd={}", fd());
    m_channel->disableAll();
    m_loop->removeChannel(m_channel.get());
}

void TcpConnection::handleRead()
{
    LOG_DEBUG("TcpConnection::handleRead fd={} state={}", fd(), static_cast<int>(m_state));
    
    if (m_state != Connected) {
        LOG_WARN("TcpConnection::handleRead called but connection is not connected");
        return;
    }
    
    ssize_t n = m_inputBuffer.readFd(fd());
    
    LOG_DEBUG("TcpConnection::handleRead read {} bytes", n);
    
    if (n > 0) {
        LOG_DEBUG("TcpConnection::handleRead has {} readable bytes", m_inputBuffer.readableBytes());
        
        if (m_readCallback) {
            m_readCallback(shared_from_this(), m_inputBuffer);
        } else {
            LOG_WARN("TcpConnection::handleRead no read callback set");
        }
    } else if (n == 0) {
        LOG_INFO("TcpConnection fd={} peer closed connection", fd());
        handleClose();
    } else {
        LOG_ERROR("TcpConnection handleRead error fd={}, errno={} ({})", 
                  fd(), errno, strerror(errno));
        
        // 只关闭连接，不重复处理
        if (m_state == Connected) {
            handleClose();
        }
    }
}
void TcpConnection::handleWrite()
{
    ssize_t n = m_outputBuffer.writeFd(fd());

    if (n > 0) {
        if (m_outputBuffer.readableBytes() == 0) {
            m_channel->disableWriting();

            if (m_state == Disconnecting) {
                ::shutdown(fd(), SHUT_WR);
            }
        }
    } else if (n < 0) {
        LOG_ERROR("TcpConnection handleWrite error fd={}", fd());
        handleClose();
    }
}

void TcpConnection::handleClose()
{
    if (m_state == Closed) return;

    LOG_INFO("TcpConnection fd={} closed", fd());

    m_state = Closed;
    m_channel->disableAll();

    if (m_closeCallback) {
        m_closeCallback(shared_from_this());
    }
}

void TcpConnection::send(const std::string& data)
{
    if (m_state != Connected) return;

    sendInLoop(data);
}

void TcpConnection::shutdown()
{
    if (m_state == Connected) {
        m_state = Disconnecting;

        if (m_outputBuffer.readableBytes() == 0) {
            ::shutdown(fd(), SHUT_WR);
        }
    }
}

void TcpConnection::sendInLoop(const std::string& data)
{
    ssize_t n = 0;

    if (m_outputBuffer.readableBytes() == 0) {
        n = ::write(fd(), data.data(), data.size());
        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG_ERROR("TcpConnection write error fd={}", fd());
                handleClose();
                return;
            }
            n = 0;
        }
    }

    if (static_cast<size_t>(n) < data.size()) {
        m_outputBuffer.append(data.data() + n, data.size() - n);
        m_channel->enableWriting();
    }
}

int TcpConnection::fd() const
{
    return m_channel->fd();
}
