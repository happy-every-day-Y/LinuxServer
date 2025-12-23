#include "Session.h"
#include "Connection.h"
#include "Logger.h"

Session::Session(uint64_t id) : m_id(id) {
    LOG_INFO("Created, sid={}", m_id);
}

uint64_t Session::id() const {
    return m_id;
}

void Session::set(const std::string& key, std::any value) {
    LOG_DEBUG("set called, sid={}, key={}", m_id, key);
    std::lock_guard lock(m_mutex);
    m_data[key] = std::move(value);
}

std::any Session::get(const std::string& key) const {
    LOG_DEBUG("get called, sid={}, key={}", m_id, key);
    std::lock_guard lock(m_mutex);
    auto it = m_data.find(key);
    return it != m_data.end() ? it->second : std::any{};
}

bool Session::has(const std::string& key) const {
    LOG_DEBUG("has called, sid={}, key={}", m_id, key);
    std::lock_guard lock(m_mutex);
    return m_data.count(key);
}

void Session::attach(const std::shared_ptr<Connection>& conn) {
    LOG_INFO("attach called, sid={}, conn={}", m_id, static_cast<const void*>(conn.get()));
    std::lock_guard lock(m_mutex);
    m_connections.insert(conn);
}

void Session::detach(const std::shared_ptr<Connection>& conn) {
    LOG_INFO("detach called, sid={}, conn={}", m_id, static_cast<const void*>(conn.get()));
    std::lock_guard lock(m_mutex);
    m_connections.erase(conn);
}

bool Session::empty() const {
    LOG_DEBUG("empty called, sid={}", m_id);
    std::lock_guard lock(m_mutex);
    return m_connections.empty();
}
