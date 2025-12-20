// FILE: ./logic/handler/HttpHandler.cpp
#include "HttpHandler.h"
#include "Logger.h"

HttpHandler::HttpHandler(const std::string& name,
                         const std::vector<std::string>& patterns,
                         const std::vector<HttpRequest::Method>& methods,
                         HttpHandlerFunc func)
    : m_name(name), m_patterns(patterns), m_methods(methods), m_handlerFunc(std::move(func)) {}

std::shared_ptr<HttpResponse> HttpHandler::handleHttp(
    const std::shared_ptr<HttpRequest>& request,
    const std::shared_ptr<Session>& session) {
    
    LOG_DEBUG("HttpHandler '{}' handling request: {} {}", 
              m_name, 
              request->path,
              request->getHeader("User-Agent"));
    
    try {
        return m_handlerFunc(request, session);
    } catch (const std::exception& e) {
        LOG_ERROR("HttpHandler '{}' error: {}", m_name, e.what());
        return HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                   "Internal Server Error: " + std::string(e.what()));
    }
}