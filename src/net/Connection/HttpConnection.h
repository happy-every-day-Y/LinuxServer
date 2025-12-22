#pragma once
#include "Connection.h"
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <atomic>

class HttpConnection
    : public Connection {
public:
    using tcp = boost::asio::ip::tcp;

    explicit HttpConnection(tcp::socket socket);
    ~HttpConnection();

    void start() override;
    void send(const std::string& data) override;
    void close() override;
    std::string remoteAddr() const override;

private:
    void doRead();
    void onRead(boost::system::error_code ec, std::size_t bytes);
    void handleRequest();

private:
    tcp::socket m_socket;
    boost::beast::flat_buffer m_buffer;
    boost::beast::http::request<boost::beast::http::string_body> m_request;

    // 🔑 关闭状态（必须有）
    std::atomic_bool m_closed{false};
};
