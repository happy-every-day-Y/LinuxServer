// WebSocketConnection.h
#pragma once

#include "Connection.h"
#include "TcpConnection.h"
#include <boost/beast/websocket.hpp>
#include <memory>
#include <string>

class WebSocketConnection : public Connection {
public:
    using Ptr = std::shared_ptr<WebSocketConnection>;

    explicit WebSocketConnection(const std::shared_ptr<TcpConnection>& tcp);

    void start() override;
    void send(const std::string& message) override;
    std::string remoteAddr() const override;
    void close() override;

private:
    void onRawData(boost::beast::flat_buffer& buffer);
    void handleHandshake();
    void handleFrame(const std::string& msg);

private:
    std::shared_ptr<TcpConnection> m_tcp;
    bool m_handshakeDone{false};
    boost::beast::flat_buffer m_buffer;
};
