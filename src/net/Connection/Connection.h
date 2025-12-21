#pragma once

#include <memory>
#include <string>
#include "Session.h"

class Session;

class Connection : public std::enable_shared_from_this<Connection>{
public:
    using Ptr = std::shared_ptr<Connection>;
    virtual ~Connection() = default;

    virtual void send(const std::string& data) = 0;
    virtual std::string remoteAddr() const = 0;
    virtual void close() = 0;

    void bindSession(const std::shared_ptr<Session>& session);
    std::shared_ptr<Session> getSession() const;

protected:
    std::weak_ptr<Session> m_session;
};