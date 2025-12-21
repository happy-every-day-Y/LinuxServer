#pragma once
#include "Connection.h"
#include "TcpConnection.h"
#include <boost/beast.hpp>
#include <memory>

class HttpConnection : public Connection {
public:
    using Ptr = std::shared_ptr<HttpConnection>;

    explicit HttpConnection(const std::shared_ptr<TcpConnection>& tcp);

    void start() override;
    void send(const std::string& data) override;
    std::string remoteAddr() const override;
    void close() override;

private:
    void onRawData(boost::beast::flat_buffer& buffer);
    void handleRequest();
    
private:
    std::shared_ptr<TcpConnection> m_tcp;
    boost::beast::http::request_parser<boost::beast::http::string_body> m_parser;
    boost::beast::http::request<boost::beast::http::string_body> m_request;
    boost::beast::flat_buffer m_buffer;
};
