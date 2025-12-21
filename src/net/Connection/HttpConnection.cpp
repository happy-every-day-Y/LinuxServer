#include "HttpConnection.h"
#include <boost/beast/http.hpp>
#include <iostream>
#include <sstream>

namespace http = boost::beast::http;

HttpConnection::HttpConnection(const std::shared_ptr<TcpConnection>& tcp)
    : m_tcp(tcp) {
    m_parser.body_limit(1024 * 1024);
}

void HttpConnection::start() {
    auto self = std::static_pointer_cast<HttpConnection>(shared_from_this());

    m_tcp->setOnMessage([self](boost::beast::flat_buffer& buffer) {
        self->onRawData(buffer);
    });

    m_tcp->setOnClose([self]() {
        self->close();
    });
}

void HttpConnection::onRawData(boost::beast::flat_buffer& buffer) {
    boost::beast::http::request_parser<boost::beast::http::string_body> parser;
    parser.body_limit(1024*1024);

    boost::system::error_code ec;
    parser.put(buffer.data(), ec);
    if (ec == boost::beast::http::error::need_more)
        return;
    if (ec) {
        std::cerr << "HTTP parse error: " << ec.message() << std::endl;
        close();
        return;
    }

    if (parser.is_done()) {
        m_request = parser.release();
        buffer.consume(buffer.size());
        handleRequest();
    }
}


void HttpConnection::handleRequest() {
    http::response<http::string_body> resp;
    resp.version(m_request.version());
    resp.keep_alive(m_request.keep_alive());
    resp.result(http::status::ok);
    resp.set(http::field::server, "MyCppServer");
    resp.body() = "Hello from HttpConnection\n";
    resp.prepare_payload();

    // 发送 response
    std::ostringstream oss;
    oss << resp;
    send(oss.str());

    if (!resp.keep_alive()) {
        close();
    }
}

void HttpConnection::send(const std::string& data) {
    m_tcp->sendRaw(data);
}

std::string HttpConnection::remoteAddr() const {
    return m_tcp->remoteAddr();
}

void HttpConnection::close() {
    if (auto session = getSession()) {
        session->detach(shared_from_this());
    }

    m_tcp->close();
}
