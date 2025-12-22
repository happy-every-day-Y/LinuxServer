#include "WebSocketConnection.h"
#include "Logger.h"

namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

WebSocketConnection::WebSocketConnection(
    tcp::socket socket,
    boost::beast::http::request<boost::beast::http::string_body> req)
    : m_ws(std::move(socket)), m_request(std::move(req)) {

    LOG_INFO("[WebSocketConnection] Created, this={}",
             static_cast<void*>(this));
}

void WebSocketConnection::start() {
    auto self = std::static_pointer_cast<WebSocketConnection>(shared_from_this());

    m_ws.set_option(websocket::stream_base::timeout::suggested(
        boost::beast::role_type::server));
    m_ws.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& res) {
            res.set(boost::beast::http::field::server, "Beast-WebSocket");
        }));

    m_ws.async_accept(
        m_request,
        [self](boost::system::error_code ec) {
            if (ec) {
                self->fail(ec, "accept");
                return;
            }
            LOG_INFO("[WebSocketConnection] handshake success");
            self->doRead();
        });
}

void WebSocketConnection::doRead() {
    auto self = std::static_pointer_cast<WebSocketConnection>(shared_from_this());
    m_ws.async_read(
        m_buffer,
        [self](boost::system::error_code ec, std::size_t bytes) {
            self->onRead(ec, bytes);
        });
}

void WebSocketConnection::onRead(boost::system::error_code ec,
                                 std::size_t bytes) {
    if (ec) {
        fail(ec, "read");
        return;
    }

    std::string msg = boost::beast::buffers_to_string(
        m_buffer.data());

    LOG_INFO("[WebSocketConnection] recv: {}", msg);

    m_ws.text(true);
    send("Echo: " + msg);

    m_buffer.consume(bytes);
    doRead();
}

void WebSocketConnection::send(const std::string& msg) {
    auto self = std::static_pointer_cast<WebSocketConnection>(shared_from_this());
    m_ws.async_write(
        boost::asio::buffer(msg),
        [self](boost::system::error_code ec, std::size_t) {
            if (ec) self->fail(ec, "write");
        });
}

void WebSocketConnection::close() {
    boost::system::error_code ec;
    m_ws.close(websocket::close_code::normal, ec);
}

std::string WebSocketConnection::remoteAddr() const {
    return m_ws.next_layer().remote_endpoint().address().to_string();
}

void WebSocketConnection::fail(boost::system::error_code ec,
                               const std::string& where) {
    LOG_WARN("[WebSocketConnection] {} error: {}", where, ec.message());
}
