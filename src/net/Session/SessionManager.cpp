#include "SessionManager.h"
#include "Logger.h"

SessionManager::SessionManager()
    : m_nextSessionId(1) {
    LOG_INFO("Created");
}

SessionManager::SessionPtr SessionManager::createSession() {
    uint64_t id = m_nextSessionId.fetch_add(1, std::memory_order_relaxed);
    LOG_INFO("Creating session, id={}", id);

    auto session = std::make_shared<Session>(id);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.emplace(id, session);
        LOG_DEBUG("Session stored, id={}", id);
    }

    return session;
}

SessionManager::SessionPtr SessionManager::getSession(uint64_t sessionId) {
    LOG_DEBUG("getSession called, id={}", sessionId);

    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessions.find(sessionId);
    if (it != m_sessions.end()) {
        LOG_DEBUG("Session found, id={}", sessionId);
        return it->second;
    }

    LOG_DEBUG("Session not found, id={}", sessionId);
    return nullptr;
}

void SessionManager::tryRemoveSession(const SessionPtr& session) {
    if (!session) {
        LOG_DEBUG("tryRemoveSession called with nullptr");
        return;
    }

    LOG_DEBUG("tryRemoveSession called, sid={}", session->id());

    // 关键点：Session 自己知道有没有 Connection
    if (!session->empty()) {
        LOG_DEBUG("Session not empty, sid={}", session->id());
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(session->id());
    LOG_INFO("Session removed, sid={}", session->id());
}

void SessionManager::removeSession(uint64_t sessionId) {
    LOG_INFO("removeSession called, id={}", sessionId);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(sessionId);
}

void SessionManager::removeAllSessions() {
    LOG_INFO("removeAllSessions called");

    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.clear();
}

size_t SessionManager::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t sz = m_sessions.size();
    LOG_DEBUG("size queried, count={}", sz);
    return sz;
}
