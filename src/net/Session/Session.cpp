#include "Session.h"
#include "Connection.h"

Session::Session(uint64_t id) : m_id(id) {}

uint64_t Session::id() const {
    return m_id;
}

void Session::set(const std::string& key, std::any value) {
    std::lock_guard lock(m_mutex);
    m_data[key] = std::move(value);
}

std::any Session::get(const std::string& key) const {
    std::lock_guard lock(m_mutex);
    auto it = m_data.find(key);
    return it != m_data.end() ? it->second : std::any{};
}

bool Session::has(const std::string& key) const {
    std::lock_guard lock(m_mutex);
    return m_data.count(key);
}

void Session::attach(const std::shared_ptr<Connection>& conn) {
    std::lock_guard lock(m_mutex);
    m_connections.insert(conn);
}

void Session::detach(const std::shared_ptr<Connection>& conn) {
    std::lock_guard lock(m_mutex);
    m_connections.erase(conn);
}

bool Session::empty() const {
    std::lock_guard lock(m_mutex);
    return m_connections.empty();
}
