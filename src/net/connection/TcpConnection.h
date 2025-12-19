#pragma once
#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"
#include <memory>
#include <functional>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    using ConnectionCallback = std::function<void(const Ptr&)>;
    using MessageCallback = std::function<void(const Ptr&, Buffer&)>;
    using CloseCallback = std::function<void(const Ptr&)>;

    TcpConnection(EventLoop* loop, int fd);
    ~TcpConnection();

    void handleRead();
    void handleWrite();
    void handleClose();

    void send(const std::string& msg);
    void shutdown();

    void setConnectionCallback(ConnectionCallback cb) { m_connectionCallback = std::move(cb); }
    void setMessageCallback(MessageCallback cb) { m_messageCallback = std::move(cb); }
    void setCloseCallback(CloseCallback cb) { m_closeCallback = std::move(cb); }

    int fd() const;
private:
    EventLoop* m_loop;
    std::unique_ptr<Channel> m_channel;
    Buffer m_inputBuffer;
    Buffer m_outputBuffer;
    
    enum State {Connecting, Connected, Disconnecting, Closed} m_state;

    ConnectionCallback m_connectionCallback;
    MessageCallback m_messageCallback;
    CloseCallback m_closeCallback;

    void sendInLoop(const std::string& msg);
};