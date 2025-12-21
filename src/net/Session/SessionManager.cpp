#include "SessionManager.h"

SessionManager::SessionManager()
    : m_nextSessionId(1) {}

SessionManager::SessionPtr SessionManager::createSession() {
    uint64_t id = m_nextSessionId.fetch_add(1, std::memory_order_relaxed);

    auto session = std::make_shared<Session>(id);

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_sessions.emplace(id, session);
    }

    return session;
}

SessionManager::SessionPtr SessionManager::getSession(uint64_t sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessions.find(sessionId);
    if (it != m_sessions.end()) {
        return it->second;
    }
    return nullptr;
}

void SessionManager::tryRemoveSession(const SessionPtr& session) {
    if (!session) return;

    // 关键点：Session 自己知道有没有 Connection
    if (!session->empty()) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(session->id());
}

void SessionManager::removeSession(uint64_t sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.erase(sessionId);
}

void SessionManager::removeAllSessions()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessions.clear();
}

size_t SessionManager::size() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.size();
}
