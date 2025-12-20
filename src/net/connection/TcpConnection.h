#pragma once

#include <memory>
#include <functional>
#include <string>
#include <sys/socket.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Buffer.h"

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;

    using ConnectionCallback = std::function<void(const Ptr&)>;
    using ReadCallback       = std::function<void(const Ptr&, Buffer&)>;
    using CloseCallback      = std::function<void(const Ptr&)>;

    TcpConnection(EventLoop* loop, int fd);
    ~TcpConnection();

    void handleRead();
    void handleWrite();
    void handleClose();

    void send(const std::string& data);
    void shutdown();

    void setConnectionCallback(ConnectionCallback cb) { m_connectionCallback = std::move(cb); }
    void setReadCallback(ReadCallback cb)             { m_readCallback = std::move(cb); }
    void setCloseCallback(CloseCallback cb)           { m_closeCallback = std::move(cb); }

    int fd() const;

private:
    enum State {
        Connecting,
        Connected,
        Disconnecting,
        Closed
    };

    void sendInLoop(const std::string& data);

private:
    EventLoop* m_loop;
    std::unique_ptr<Channel> m_channel;

    Buffer m_inputBuffer;
    Buffer m_outputBuffer;

    State m_state;

    ConnectionCallback m_connectionCallback;
    ReadCallback       m_readCallback;
    CloseCallback      m_closeCallback;
};
