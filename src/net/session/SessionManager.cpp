#include "SessionManager.h"
#include "Logger.h"

std::unordered_map<int, std::shared_ptr<Session>> SessionManager::m_sessionsByFd;
std::unordered_map<int, std::vector<std::weak_ptr<Session>>> SessionManager::m_sessionsByUserId;
std::mutex SessionManager::m_mutex;

void SessionManager::addSession(int fd, std::shared_ptr<Session> session) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessionsByFd[fd] = session;
    LOG_INFO("Session added, fd={}", fd);
}

void SessionManager::removeSession(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_sessionsByFd.find(fd);
    if (it == m_sessionsByFd.end())
        return;

    auto session = it->second;
    m_sessionsByFd.erase(it);

    if (session->isAuthenticated()) {
        int userId = session->userId();
        auto uit = m_sessionsByUserId.find(userId);
        if (uit != m_sessionsByUserId.end()) {
            auto& vec = uit->second;
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [&](const std::weak_ptr<Session>& wp) {
                        auto sp = wp.lock();
                        return !sp || sp == session;
                    }),
                vec.end()
            );

            if (vec.empty()) {
                m_sessionsByUserId.erase(uit);
            }
        }
    }

    LOG_INFO("Session removed, fd={}", fd);
}

std::shared_ptr<Session> SessionManager::getSession(int fd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessionsByFd.find(fd);
    return it != m_sessionsByFd.end() ? it->second : nullptr;
}

void SessionManager::bindUser(int userId, const std::shared_ptr<Session> &session)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sessionsByUserId[userId].push_back(session);
    LOG_INFO("Session bound to userId={}, fd={}", userId, session->connection()->fd());
}

std::vector<std::shared_ptr<Session>> SessionManager::getSessionByUserId(int userId)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<std::shared_ptr<Session>> result;

    auto it = m_sessionsByUserId.find(userId);
    if (it == m_sessionsByUserId.end())
        return result;

    auto& vec = it->second;

    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [&](const std::weak_ptr<Session>& wp) {
                auto sp = wp.lock();
                if (sp) {
                    result.push_back(sp);
                    return false;
                }
                return true;
            }),
        vec.end()
    );

    return result;
}
