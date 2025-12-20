// FILE: ./service/HttpService/HttpService.h
#pragma once
#include <memory>
#include "TcpConnection.h"
#include "HttpCodec.h"
#include "Dispatcher.h"
#include "SessionManager.h"

class HttpService : public std::enable_shared_from_this<HttpService> {
public:
    using Ptr = std::shared_ptr<HttpService>;

    HttpService(const std::string& staticPath = "");
    ~HttpService() = default;

    void onConnection(const TcpConnection::Ptr& conn);
    void onMessage(const TcpConnection::Ptr& conn, Buffer& buffer);
    void onClose(const TcpConnection::Ptr& conn);
    
    // 注册自定义路由
    template<typename Func>
    void registerRoute(const std::string& name,
                       const std::vector<std::string>& patterns,
                       const std::vector<HttpRequest::Method>& methods,
                       Func handler) {
        Dispatcher::instance().registerHttpHandler(name, patterns, methods, handler);
    }
    
    // 快捷方法：注册GET路由
    template<typename Func>
    void get(const std::string& pattern, Func handler) {
        registerRoute("GET_" + pattern, {pattern}, 
                      {HttpRequest::Method::GET}, handler);
    }
    
    // 快捷方法：注册POST路由
    template<typename Func>
    void post(const std::string& pattern, Func handler) {
        registerRoute("POST_" + pattern, {pattern}, 
                      {HttpRequest::Method::POST}, handler);
    }

private:
    void processRequest(const TcpConnection::Ptr& conn, Buffer& buffer);
    void registerDefaultHandlers();
    std::shared_ptr<HttpResponse> handleStaticFile(const std::shared_ptr<HttpRequest>& req);

private:
    HttpCodec m_codec;
    std::string m_staticPath;
};