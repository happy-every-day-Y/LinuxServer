// FILE: ./service/HttpService/HttpService.cpp
#include "HttpService.h"
#include "Logger.h"
#include "Thread_pool.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

HttpService::HttpService(const std::string& staticPath)
    : m_codec(),
      m_staticPath(staticPath) {
    LOG_INFO("HttpService created with static path: {}", staticPath);
    
    // 注册默认处理器
    registerDefaultHandlers();
}

void HttpService::registerDefaultHandlers() {
    // 注意：静态文件处理器放在最后
    
    // 默认404处理器
    Dispatcher::instance().setDefault404Handler(
        [](const std::shared_ptr<HttpRequest>& req,
           const std::shared_ptr<Session>& session) {
            std::stringstream ss;
            ss << "<html><head><title>404 Not Found</title></head>"
               << "<body><h1>404 Not Found</h1>"
               << "<p>The requested URL " << req->path 
               << " was not found on this server.</p>"
               << "</body></html>";
            
            auto resp = HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                            ss.str(), "text/html");
            return resp;
        }
    );
    
    // 静态文件服务（放在最后，只匹配没有其他处理器的路径）
    Dispatcher::instance().registerHttpHandler(
        "static",
        {"/*"},
        {HttpRequest::Method::GET},
        [this](const std::shared_ptr<HttpRequest>& req,
               const std::shared_ptr<Session>& session) {
            return handleStaticFile(req);
        }
    );
}

void HttpService::onConnection(const TcpConnection::Ptr& conn) {
    LOG_INFO("HttpService new connection fd={}", conn->fd());

    conn->setReadCallback(
        [self = shared_from_this()](const TcpConnection::Ptr& c, Buffer& buf) {
            self->onMessage(c, buf);
        }
    );

    conn->setCloseCallback(
        [self = shared_from_this()](const TcpConnection::Ptr& c) {
            self->onClose(c);
        }
    );
}

void HttpService::onMessage(const TcpConnection::Ptr& conn, Buffer& buffer) {
    // 异步处理请求
    ThreadPool::detach_task([this, conn, buffer = std::move(buffer)]() mutable {
        processRequest(conn, buffer);
    });
}

void HttpService::processRequest(const TcpConnection::Ptr& conn, Buffer& buffer) {
    try {
        auto requests = m_codec.decode(buffer);
        
        for (auto& msg : requests) {
            if (msg->type() == MessageType::HTTP_REQUEST) {
                auto req = std::dynamic_pointer_cast<HttpRequest>(msg);
                if (!req) continue;
                
                // 获取会话
                auto session = SessionManager::getSession(conn->fd());
                if (!session) {
                    LOG_ERROR("No session found for fd={}", conn->fd());
                    continue;
                }
                
                // 设置客户端地址
                sockaddr_in addr = session->peerAddr();
                req->clientIp = inet_ntoa(addr.sin_addr);
                req->clientPort = ntohs(addr.sin_port);
                
                // 分发请求
                auto resp = Dispatcher::instance().dispatchHttp(req, session);
                if (!resp) {
                    resp = HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                               "Internal Server Error");
                }
                
                // 编码响应
                Buffer outBuf;
                m_codec.encode(resp, outBuf);
                
                // 发送响应（需要在IO线程中执行）
                conn->send(outBuf.retrieveAllAsString());
                
                // 检查是否保持连接
                if (!req->keepAlive()) {
                    conn->shutdown();
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error processing request: {}", e.what());
        
        // 发送错误响应
        auto resp = HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                        "Internal Server Error");
        Buffer outBuf;
        m_codec.encode(resp, outBuf);
        conn->send(outBuf.retrieveAllAsString());
    }
}

void HttpService::onClose(const TcpConnection::Ptr& conn) {
    LOG_INFO("HttpService connection closed fd={}", conn->fd());
}

std::shared_ptr<HttpResponse> HttpService::handleStaticFile(
    const std::shared_ptr<HttpRequest>& req) {
    
    if (m_staticPath.empty()) {
        return HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                   "Static file service not configured");
    }
    
    // 安全验证：防止路径遍历攻击
    std::string requestPath = req->path;
    if (requestPath.find("..") != std::string::npos) {
        return HttpResponse::create(HttpResponse::StatusCode::FORBIDDEN,
                                   "Forbidden");
    }
    
    // 构建文件路径
    std::string filePath = m_staticPath + requestPath;
    if (requestPath == "/") {
        filePath += "index.html";
    }
    
    // 检查文件是否存在
    if (!fs::exists(filePath) || !fs::is_regular_file(filePath)) {
        return HttpResponse::create(HttpResponse::StatusCode::NOT_FOUND,
                                   "File not found");
    }
    
    
    // 读取文件
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return HttpResponse::create(HttpResponse::StatusCode::INTERNAL_SERVER_ERROR,
                                   "Cannot read file");
    }
    
    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    
    // 根据扩展名设置Content-Type
    std::string contentType = "application/octet-stream";
    std::string ext = fs::path(filePath).extension().string();
    
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".txt", "text/plain"},
        {".pdf", "application/pdf"}
    };
    
    auto it = mimeTypes.find(ext);
    if (it != mimeTypes.end()) {
        contentType = it->second;
    }
    
    auto resp = HttpResponse::create(HttpResponse::StatusCode::OK, content, contentType);
    resp->setHeader("Cache-Control", "public, max-age=3600");
    
    return resp;
}