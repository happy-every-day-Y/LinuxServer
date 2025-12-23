#include "HttpConnection.h"
#include "WebSocketConnection.h"
#include "Logger.h"
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

HttpConnection::HttpConnection(tcp::socket socket)
    : m_socket(std::move(socket)) {
    LOG_INFO("Created, this={}, remote={}",
             static_cast<void*>(this),
             remoteAddr());
}

HttpConnection::~HttpConnection() {
    LOG_INFO("Destroyed, this={}",
             static_cast<void*>(this));
}

void HttpConnection::start() {
    LOG_DEBUG("start, this={}", static_cast<void*>(this));
    doRead();
}

void HttpConnection::doRead() {
    LOG_DEBUG("doRead, this={}", static_cast<void*>(this));

    auto self = std::static_pointer_cast<HttpConnection>(shared_from_this());
    http::async_read(
        m_socket,
        m_buffer,
        m_request,
        [self](boost::system::error_code ec, std::size_t bytes) {
            self->onRead(ec, bytes);
        });
}

void HttpConnection::onRead(boost::system::error_code ec, std::size_t bytes) {
    LOG_DEBUG("onRead, this={}, bytes={}",
              static_cast<void*>(this), bytes);

    if (ec) {
        LOG_WARN("read error, this={}, ec={}",
                 static_cast<void*>(this), ec.message());
        close();
        return;
    }

    // ---- WebSocket Upgrade ----
    if (boost::beast::websocket::is_upgrade(m_request)) {
        LOG_INFO("WebSocket upgrade, this={}",
                 static_cast<void*>(this));

        auto ws = std::make_shared<WebSocketConnection>(
            std::move(m_socket),
            std::move(m_request)
        );

        // 🔑 继承 Session
        if (auto s = getSession()) {
            ws->bindSession(s);
            LOG_DEBUG("session moved to WS, sid={}", s->id());
        }

        ws->start();
        return;
    }

    handleRequest();
}

void HttpConnection::handleRequest() {
    LOG_INFO("handleRequest, this={}, target={}",
             static_cast<void*>(this),
             m_request.target());

    // 🔑 response 必须活到 async_write 完成
    auto res = std::make_shared<http::response<http::string_body>>();
    res->version(m_request.version());
    res->keep_alive(false);
    res->result(http::status::ok);
    res->set(http::field::server, "BeastServer");
    res->body() = "Hello HTTP";
    res->prepare_payload();

    auto self = shared_from_this();
    http::async_write(
        m_socket,
        *res,
        [self, res](boost::system::error_code ec, std::size_t bytes) {
            LOG_DEBUG("write done, this={}, bytes={}",
                      static_cast<void*>(self.get()), bytes);

            if (ec) {
                LOG_WARN("write error, ec={}", ec.message());
            }

            self->close();
        });
}


void HttpConnection::send(const std::string&) {
    LOG_WARN("send() ignored (HTTP), this={}",
             static_cast<void*>(this));
}

void HttpConnection::close() {
    // 🔑 幂等：只允许关一次
    if (m_closed.exchange(true)) {
        return;
    }

    LOG_INFO("close, this={}", static_cast<void*>(this));

    // 🔑 通知 Session（此时 shared_ptr 仍然安全）
    if (auto s = getSession()) {
        s->detach(shared_from_this());
        LOG_DEBUG("detached from session, sid={}", s->id());
    }

    boost::system::error_code ec;
    m_socket.shutdown(tcp::socket::shutdown_both, ec);
    m_socket.close(ec);
}

std::string HttpConnection::remoteAddr() const {
    boost::system::error_code ec;
    auto ep = m_socket.remote_endpoint(ec);
    return ec ? "unknown" : ep.address().to_string();
}
