// FILE: ./logic/dispatcher/Dispatcher.h
#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include "Handler.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebSocketFrame.h"
#include "Session.h"

class Dispatcher {
public:
    static Dispatcher& instance();
    
    // 注册处理器
    void registerHandler(std::shared_ptr<Handler> handler);
    
    // 注册HTTP处理器（快捷方式）
    void registerHttpHandler(const std::string& name,
                            const std::vector<std::string>& patterns,
                            const std::vector<HttpRequest::Method>& methods,
                            std::function<std::shared_ptr<HttpResponse>(
                                const std::shared_ptr<HttpRequest>&,
                                const std::shared_ptr<Session>&)> func);
    
    // 分发HTTP请求
    std::shared_ptr<HttpResponse> dispatchHttp(
        const std::shared_ptr<HttpRequest>& request,
        const std::shared_ptr<Session>& session);
    
    // 分发WebSocket消息
    void dispatchWebSocket(
        const std::shared_ptr<WebSocketFrame>& frame,
        const std::shared_ptr<Session>& session);
    
    // 查找匹配的处理器
    std::shared_ptr<Handler> findHandler(const std::shared_ptr<HttpRequest>& request);
    
    // 添加路由前缀（用于API版本控制等）
    void addRoutePrefix(const std::string& prefix);
    
    // 设置默认404处理器
    void setDefault404Handler(std::function<std::shared_ptr<HttpResponse>(
        const std::shared_ptr<HttpRequest>&,
        const std::shared_ptr<Session>&)> handler);
    
private:
    Dispatcher() = default;
    
    std::vector<std::shared_ptr<Handler>> m_handlers;
    std::vector<std::string> m_routePrefixes;
    std::unordered_map<std::string, std::shared_ptr<Handler>> m_handlerCache;
    mutable std::mutex m_mutex;
    
    std::function<std::shared_ptr<HttpResponse>(
        const std::shared_ptr<HttpRequest>&,
        const std::shared_ptr<Session>&)> m_default404Handler;
    
    // 匹配路径
    bool matchAndParsePath(const std::shared_ptr<HttpRequest>& request,
                          const std::shared_ptr<Handler>& handler) const;
};