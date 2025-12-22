#pragma once
#include "Connection.h"
#include <boost/beast/websocket.hpp>
#include <boost/beast/core.hpp>

class WebSocketConnection
    : public Connection {
public:
    explicit WebSocketConnection(
        boost::asio::ip::tcp::socket socket,
        boost::beast::http::request<boost::beast::http::string_body> req);

    void start() override;
    void send(const std::string& msg) override;
    void close() override;
    std::string remoteAddr() const override;

private:
    void doRead();
    void onRead(boost::system::error_code ec, std::size_t bytes);
    void fail(boost::system::error_code ec, const std::string& where);

private:
    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> m_ws;
    boost::beast::flat_buffer m_buffer;
    boost::beast::http::request<boost::beast::http::string_body> m_request;
};
