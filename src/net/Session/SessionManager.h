#pragma once

#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <cstdint>

#include "Session.h"

class SessionManager {
public:
    using SessionPtr = std::shared_ptr<Session>;

    SessionManager();

    // 创建一个新 Session
    SessionPtr createSession();

    // 通过 id 获取 Session
    SessionPtr getSession(uint64_t sessionId);

    // 当 Connection 断开后调用，用于尝试回收 Session
    void tryRemoveSession(const SessionPtr& session);

    // 强制移除（例如超时、踢人）
    void removeSession(uint64_t sessionId);
    void removeAllSessions();

    size_t size() const;

private:
    std::atomic<uint64_t> m_nextSessionId;
    std::unordered_map<uint64_t, SessionPtr> m_sessions;
    mutable std::mutex m_mutex;
};
