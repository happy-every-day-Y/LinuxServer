#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include <sys/epoll.h>

Channel::Channel(EventLoop *loop, int fd)
: m_loop(loop), m_fd(fd), m_events(0), m_revents(0)
{
    LOG_DEBUG("Channel created, fd={}", m_fd);
}

void Channel::handleEvent()
{
    LOG_DEBUG("Channel handleEvent, fd={}, revents={:#x}", m_fd, m_revents);

    if (m_revents & EPOLLIN) {
        LOG_DEBUG("Channel fd={} EPOLLIN triggered", m_fd);
        if (m_readCallback) {
            m_readCallback();
        } else {
            LOG_WARN("Channel fd={} EPOLLIN but readCallback is null", m_fd);
        }
    }

    if (m_revents & EPOLLOUT) {
        LOG_DEBUG("Channel fd={} EPOLLOUT triggered", m_fd);
        if (m_writeCallback) {
            m_writeCallback();
        } else {
            LOG_WARN("Channel fd={} EPOLLOUT but writeCallback is null", m_fd);
        }
    }
}

void Channel::setReadCallback(EventCallback cb)
{
    m_readCallback = std::move(cb);
    LOG_DEBUG("Channel fd={} set read callback", m_fd);
}

void Channel::setWriteCallback(EventCallback cb)
{
    m_writeCallback = std::move(cb);
    LOG_DEBUG("Channel fd={} set write callback", m_fd);
}

int Channel::fd() const
{
    return m_fd;
}

uint32_t Channel::events() const
{
    return m_events;
}

void Channel::setRevents(uint32_t revt)
{
    m_revents = revt;
}

void Channel::enableReading()
{
    if (!(m_events & EPOLLIN)) {
        m_events |= EPOLLIN;
        LOG_DEBUG("Channel fd={} enableReading", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::enableWriting()
{
    if (!(m_events & EPOLLOUT)) {
        m_events |= EPOLLOUT;
        LOG_DEBUG("Channel fd={} enableWriting", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::disableReading()
{
    if (m_events & EPOLLIN) {
        m_events &= ~EPOLLIN;
        LOG_DEBUG("Channel fd={} disableReading", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::disableWriting()
{
    if (m_events & EPOLLOUT) {
        m_events &= ~EPOLLOUT;
        LOG_DEBUG("Channel fd={} disableWriting", m_fd);
        m_loop->updateChannel(this);
    }
}

void Channel::disableAll()
{
    if (m_events != 0) {
        m_events = 0;
        LOG_DEBUG("Channel fd={} disableAll", m_fd);
        m_loop->updateChannel(this);
    }
}