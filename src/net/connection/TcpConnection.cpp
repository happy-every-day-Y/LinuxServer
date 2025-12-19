#include "TcpConnection.h"
#include "Logger.h"

TcpConnection::TcpConnection(EventLoop *loop, int fd)
    : m_loop(loop), m_channel(new Channel(loop, fd)),
    m_inputBuffer(), m_outputBuffer(), m_state(Connecting)
{
    LOG_INFO("TcpConnection created fd={}", fd);

    m_channel->setReadCallback([this]{ handleRead(); });
    m_channel->setWriteCallback([this]{ handleWrite(); });
    m_channel->enableReading();
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection destroyed fd={}", fd());
    m_loop->removeChannel(m_channel.get());
}

void TcpConnection::handleRead()
{
    ssize_t n = m_inputBuffer.readFd(fd());
    if(n > 0){
        if(m_messageCallback){
            m_messageCallback(shared_from_this(), m_inputBuffer);
        }
    }else if(n == 0){
        handleClose();
    }else{
        LOG_ERROR("TcpConnection handleRead error fd={}", fd());
    }
}

void TcpConnection::handleWrite()
{
    ssize_t n = m_outputBuffer.writeFd(fd());
    if(n > 0){
        if(m_outputBuffer.readableBytes() == 0){
            m_channel->disableWriting();
        }
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection fd={} closed", fd());
    m_state = Closed;
    m_channel->disableAll();
    if (m_closeCallback) m_closeCallback(shared_from_this());
}

void TcpConnection::send(const std::string &msg)
{
    if(m_state == Connected){
        if(m_loop){
            sendInLoop(msg);
        }
    }
}

void TcpConnection::shutdown()
{
    if(m_state == Connected){
        m_state = Disconnecting;
        if(m_outputBuffer.readableBytes() == 0){
            ::shutdown(fd(), SHUT_WR);
        }
    }
}

void TcpConnection::sendInLoop(const std::string &msg)
{
    ssize_t n = 0;
    if(m_outputBuffer.readableBytes() == 0){
        n = ::write(fd(), msg.data(), msg.size());
        if(n < 0){
            LOG_ERROR("TcpConnection sendInLoop write fd={} error={}", fd(), n);
            n = 0;
        }
    }
    if(static_cast<size_t>(n) < msg.size()){
        m_outputBuffer.append(msg.data() + n, msg.size() - n);
        m_channel->enableWriting();
    }
}

int TcpConnection::fd() const
{
    return m_channel->fd();
}
