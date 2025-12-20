// FILE: ./logic/handler/HttpHandler.h
#pragma once
#include "Handler.h"
#include <functional>
#include <string>
#include <unordered_map>

// HTTP处理器函数类型
using HttpHandlerFunc = std::function<std::shared_ptr<HttpResponse>(
    const std::shared_ptr<HttpRequest>&,
    const std::shared_ptr<Session>&)>;

class HttpHandler : public Handler {
public:
    HttpHandler(const std::string& name,
                const std::vector<std::string>& patterns,
                const std::vector<HttpRequest::Method>& methods,
                HttpHandlerFunc func);
    
    std::shared_ptr<HttpResponse> handleHttp(
        const std::shared_ptr<HttpRequest>& request,
        const std::shared_ptr<Session>& session) override;
    
    void handleWebSocket(
        const std::shared_ptr<WebSocketFrame>& frame,
        const std::shared_ptr<Session>& session) override {
        // HTTP处理器不处理WebSocket
        (void)frame;    // 消除未使用参数警告
        (void)session;  // 消除未使用参数警告
    }
    
    std::string name() const override { return m_name; }
    std::vector<std::string> pathPatterns() const override { return m_patterns; }
    std::vector<HttpRequest::Method> supportedMethods() const override { return m_methods; }
    
private:
    std::string m_name;
    std::vector<std::string> m_patterns;
    std::vector<HttpRequest::Method> m_methods;
    HttpHandlerFunc m_handlerFunc;
};