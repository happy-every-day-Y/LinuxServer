#pragma once
#include <unordered_map>
#include <memory>
#include <mutex>
#include "Session.h"

class SessionManager {
public:

    static void addSession(int fd, std::shared_ptr<Session> session);
    static void removeSession(int fd);
    static std::shared_ptr<Session> getSession(int fd);

    static void bindUser(int userId, const std::shared_ptr<Session>& session);
    static std::vector<std::shared_ptr<Session>> getSessionByUserId(int userId);

private:
    static std::unordered_map<int, std::shared_ptr<Session>> m_sessionsByFd;
    static std::unordered_map<int, std::vector<std::weak_ptr<Session>>> m_sessionsByUserId;
    static std::mutex m_mutex;
};
