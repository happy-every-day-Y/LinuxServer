// FILE: ./logic/handler/Handler.h
#pragma once
#include <memory>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "WebSocketFrame.h"
#include "Session.h"

class Handler {
public:
    virtual ~Handler() = default;
    
    // 处理HTTP请求
    virtual std::shared_ptr<HttpResponse> handleHttp(
        const std::shared_ptr<HttpRequest>& request,
        const std::shared_ptr<Session>& session) = 0;
    
    // 处理WebSocket消息
    virtual void handleWebSocket(
        const std::shared_ptr<WebSocketFrame>& frame,
        const std::shared_ptr<Session>& session) = 0;
    
    // 获取处理器名称
    virtual std::string name() const = 0;
    
    // 支持的HTTP方法
    virtual std::vector<HttpRequest::Method> supportedMethods() const {
        return {
            HttpRequest::Method::GET,
            HttpRequest::Method::POST
        };
    }
    
    // 支持的路径模式
    virtual std::vector<std::string> pathPatterns() const = 0;
    
    // 检查路径是否匹配
    virtual bool matchPath(const std::string& path) const {
        // 默认实现：检查是否匹配任何路径模式
        for (const auto& pattern : pathPatterns()) {
            if (pattern == path || 
                (pattern.back() == '*' && path.find(pattern.substr(0, pattern.size() - 1)) == 0)) {
                return true;
            }
        }
        return false;
    }
    
protected:
    // 路径匹配辅助方法
    static bool matchPattern(const std::string& pattern, const std::string& path,
                            std::unordered_map<std::string, std::string>& params) {
        // TODO: 实现参数化路径匹配
        return pattern == path;
    }
};