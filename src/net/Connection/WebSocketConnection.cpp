// WebSocketConnection.cpp
#include "WebSocketConnection.h"
#include <iostream>
#include <sstream>

WebSocketConnection::WebSocketConnection(const std::shared_ptr<TcpConnection>& tcp)
    : m_tcp(tcp) {}

void WebSocketConnection::start() {
    auto self = std::static_pointer_cast<WebSocketConnection>(shared_from_this());

    // 绑定 TCP 回调
    m_tcp->setOnMessage([self](boost::beast::flat_buffer& buffer){
        self->onRawData(buffer);
    });

    m_tcp->setOnClose([self](){
        self->close();
    });

    m_tcp->start();
}

void WebSocketConnection::onRawData(boost::beast::flat_buffer& buffer) {
    if (!m_handshakeDone) {
        handleHandshake();
        return;
    }

    // 假设简单处理：每次收到的都是完整的文本帧
    std::string msg{
        static_cast<const char*>(buffer.data().data()),
        buffer.size()
    };
    buffer.consume(buffer.size());

    handleFrame(msg);
}

void WebSocketConnection::handleHandshake() {
    // ⚠️ 简化：这里只做一个示例，你可能需要完整解析 HTTP 握手
    m_handshakeDone = true;

    // 可以给客户端返回 101 Switching Protocols 响应
    std::string handshakeResp =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: ...\r\n\r\n";

    m_tcp->sendRaw(handshakeResp);
}

void WebSocketConnection::handleFrame(const std::string& msg) {
    // TODO: 解析 WebSocket 帧
    std::cout << "[WebSocket] Received: " << msg << std::endl;

    // 回显示例
    send(msg);
}

void WebSocketConnection::send(const std::string& message) {
    if (!m_handshakeDone) return;

    // TODO: 这里简单示例，没做 WebSocket 帧封装
    m_tcp->sendRaw(message);
}

std::string WebSocketConnection::remoteAddr() const {
    return m_tcp->remoteAddr();
}

void WebSocketConnection::close() {
    if (auto session = getSession()) {
        session->detach(shared_from_this());
    }
    m_tcp->close();
}
