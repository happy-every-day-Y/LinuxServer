#include "Connection.h"

void Connection::bindSession(const std::shared_ptr<Session> &session)
{
    if(!session) return;
    if(!m_session.expired()) return;

    m_session = session;
    session->attach(shared_from_this());
}

std::shared_ptr<Session> Connection::getSession() const
{
    return m_session.lock();
}
