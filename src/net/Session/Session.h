#pragma once
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <any>
#include <memory>
#include <string>
#include <mutex>
#include "Connection.h"

class Connection;

class Session : public std::enable_shared_from_this<Session> {
public:
    using Ptr = std::shared_ptr<Session>;

    explicit Session(uint64_t id);

    uint64_t id() const;

    // ---- 业务数据 ----
    void set(const std::string& key, std::any value);
    std::any get(const std::string& key) const;
    bool has(const std::string& key) const;

    // ---- 连接管理（重点）----
    void attach(const std::shared_ptr<Connection>& conn);
    void detach(const std::shared_ptr<Connection>& conn);
    bool empty() const;

private:
    uint64_t m_id;

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::any> m_data;
    std::unordered_set<std::shared_ptr<Connection>> m_connections;
};