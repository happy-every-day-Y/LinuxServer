#include "TcpConnection.h"

#include <boost/asio/write.hpp>
#include <iostream>

TcpConnection::TcpConnection(boost::asio::ip::tcp::socket socket)
    : m_socket(std::move(socket)) {}

void TcpConnection::start() {
    doRead();
}

void TcpConnection::setOnMessage(std::function<void(boost::beast::flat_buffer&)> cb) {
    m_onMessage = std::move(cb);
}

void TcpConnection::setOnClose(std::function<void()> cb) {
    m_onClose = std::move(cb);
}

void TcpConnection::doRead() {
    auto self = shared_from_this();

    m_socket.async_read_some(
        m_readBuffer.prepare(4096),
        [this, self](boost::system::error_code ec, std::size_t bytes) {
            if (ec) {
                close();
                return;
            }

            // 提交读到的数据
            m_readBuffer.commit(bytes);

            // 交给上层（HTTP / WS）
            if (m_onMessage) {
                m_onMessage(m_readBuffer);
            }

            // 继续读
            doRead();
        }
    );
}

void TcpConnection::sendRaw(std::string data) {
    if (m_closed) return;

    bool writing = !m_writeQueue.empty();
    m_writeQueue.emplace_back(std::move(data));

    // 如果当前没有在写，立即启动写
    if (!writing) {
        doWrite();
    }
}

void TcpConnection::doWrite() {
    if (m_closed || m_writeQueue.empty()) return;

    auto self = shared_from_this();

    boost::asio::async_write(
        m_socket,
        boost::asio::buffer(m_writeQueue.front()),
        [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
            if (ec) {
                close();
                return;
            }

            // 当前数据写完，弹出
            m_writeQueue.pop_front();

            // 如果还有待写数据，继续
            if (!m_writeQueue.empty()) {
                doWrite();
            }
        }
    );
}

void TcpConnection::close() {
    if (m_closed) return;
    m_closed = true;

    boost::system::error_code ec;

    if (m_socket.is_open()) {
        m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        m_socket.close(ec);
    }

    if (m_onClose) {
        m_onClose();
    }
}

std::string TcpConnection::remoteAddr() const {
    try {
        return m_socket.remote_endpoint().address().to_string();
    } catch (...) {
        return "unknown";
    }
}
