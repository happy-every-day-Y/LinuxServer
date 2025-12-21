#pragma once

#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <functional>
#include <memory>
#include <string>
#include <deque>

class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using Ptr = std::shared_ptr<TcpConnection>;

    explicit TcpConnection(boost::asio::ip::tcp::socket socket);

    void start();
    void sendRaw(std::string data);
    void close();

    std::string remoteAddr() const;

    // 上层注册
    void setOnMessage(std::function<void(boost::beast::flat_buffer&)> cb);
    void setOnClose(std::function<void()> cb);

private:
    void doRead();
    void doWrite();

private:
    boost::asio::ip::tcp::socket m_socket;
    boost::beast::flat_buffer m_readBuffer;

    std::deque<std::string> m_writeQueue;

    bool m_closed{false};

    std::function<void(boost::beast::flat_buffer&)> m_onMessage;
    std::function<void()> m_onClose;
};
