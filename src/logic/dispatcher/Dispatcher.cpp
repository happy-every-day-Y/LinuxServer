// FILE: ./logic/dispatcher/Dispatcher.cpp
#include "Dispatcher.h"
#include "Logger.h"
#include "HttpHandler.h"
#include <algorithm>
#include <regex>

Dispatcher& Dispatcher::instance() {
    static Dispatcher instance;
    return instance;
}

void Dispatcher::registerHandler(std::shared_ptr<Handler> handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.push_back(handler);
    LOG_INFO("Registered handler: {}", handler->name());
}

void Dispatcher::registerHttpHandler(const std::string& name,
                                     const std::vector<std::string>& patterns,
                                     const std::vector<HttpRequest::Method>& methods,
                                     std::function<std::shared_ptr<HttpResponse>(
                                         const std::shared_ptr<HttpRequest>&,
                                         const std::shared_ptr<Session>&)> func) {
    auto handler = std::make_shared<HttpHandler>(name, patterns, methods, func);
    registerHandler(handler);
}

std::shared_ptr<HttpResponse> Dispatcher::dispatchHttp(
    const std::shared_ptr<HttpRequest>& request,
    const std::shared_ptr<Session>& session) {
    
    LOG_DEBUG("Dispatching HTTP request: {} {}", 
              request->path, 
              static_cast<int>(request->method));
    
    auto handler = findHandler(request);
    if (!handler) {
        LOG_WARN("No handler found for path: {}", request->path);
        
        if (m_default404Handler) {
            return m_default404Handler(request, session);
        }
        
        // 默认404响应
        auto resp = HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                        "<html><body><h1>404 Not Found</h1></body></html>",
                                        "text/html");
        return resp;
    }
    
    // 检查方法是否支持
    auto supportedMethods = handler->supportedMethods();
    if (std::find(supportedMethods.begin(), supportedMethods.end(), 
                  request->method) == supportedMethods.end()) {
        LOG_WARN("Method not allowed for path: {}", request->path);
        auto resp = HttpResponse::create(HttpResponse::StatusCode::METHOD_NOT_ALLOWED,
                                        "Method Not Allowed");
        resp->setHeader("Allow", "GET, POST, PUT, DELETE");
        return resp;
    }
    
    return handler->handleHttp(request, session);
}

void Dispatcher::dispatchWebSocket(
    const std::shared_ptr<WebSocketFrame>& frame,
    const std::shared_ptr<Session>& session) {
    
    LOG_DEBUG("Dispatching WebSocket frame, opcode: {}", 
              static_cast<int>(frame->opcode));
    
    // TODO: 实现WebSocket消息分发
    // 这里需要根据session或frame内容找到对应的处理器
    
    (void)frame;    // 消除未使用参数警告
    (void)session;  // 消除未使用参数警告
}

std::shared_ptr<Handler> Dispatcher::findHandler(
    const std::shared_ptr<HttpRequest>& request) {
    
    // 检查缓存
    std::string cacheKey = request->path + "_" + 
                          std::to_string(static_cast<int>(request->method));
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlerCache.find(cacheKey);
        if (it != m_handlerCache.end()) {
            return it->second;
        }
    }
    
    // 遍历所有处理器查找匹配
    for (const auto& handler : m_handlers) {
        if (matchAndParsePath(request, handler)) {
            // 缓存结果
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handlerCache[cacheKey] = handler;
            return handler;
        }
    }
    
    return nullptr;
}

void Dispatcher::addRoutePrefix(const std::string& prefix) {
    m_routePrefixes.push_back(prefix);
}

void Dispatcher::setDefault404Handler(
    std::function<std::shared_ptr<HttpResponse>(
        const std::shared_ptr<HttpRequest>&,
        const std::shared_ptr<Session>&)> handler) {
    m_default404Handler = std::move(handler);
}

bool Dispatcher::matchAndParsePath(const std::shared_ptr<HttpRequest>& request,
                                   const std::shared_ptr<Handler>& handler) const {
    
    // 首先检查处理器是否支持该HTTP方法
    auto supportedMethods = handler->supportedMethods();
    if (std::find(supportedMethods.begin(), supportedMethods.end(), 
                  request->method) == supportedMethods.end()) {
        return false;
    }
    
    // 检查路径模式
    for (const auto& pattern : handler->pathPatterns()) {
        // 应用路由前缀
        std::string fullPattern = pattern;
        for (const auto& prefix : m_routePrefixes) {
            if (request->path.find(prefix) == 0) {
                // 可能需要调整匹配逻辑
            }
        }
        
        // 简单字符串匹配（TODO: 可以改为正则表达式或参数化路径匹配）
        if (pattern == request->path || 
            (pattern.back() == '*' && request->path.find(pattern.substr(0, pattern.size() - 1)) == 0)) {
            return true;
        }
        
        // 尝试参数化路径匹配：/user/{id}
        if (pattern.find('{') != std::string::npos) {
            // 简化实现：将 {id} 替换为正则表达式 ([^/]+)
            std::regex regexPattern(std::regex_replace(pattern, 
                std::regex("\\{[^}]+\\}"), "([^/]+)"));
            
            if (std::regex_match(request->path, regexPattern)) {
                // TODO: 提取路径参数并设置到request中
                return true;
            }
        }
    }
    
    return false;
}