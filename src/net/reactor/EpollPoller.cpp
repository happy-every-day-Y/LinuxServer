#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>

static const int kMaxEvents = 1024;

EpollPoller::EpollPoller()
{
    m_epollfd = epoll_create1(EPOLL_CLOEXEC);
    if (m_epollfd < 0) {
        LOG_ERROR("epoll_create1 failed: {}", strerror(errno));
    } else {
        LOG_INFO("EpollPoller created, epollfd={}", m_epollfd);
    }
}

EpollPoller::~EpollPoller()
{
    LOG_INFO("EpollPoller destroyed, epollfd={}", m_epollfd);
    close(m_epollfd);
}

void EpollPoller::poll(int timeoutMs, std::vector<Channel *> &activeChannels)
{
    epoll_event events[kMaxEvents];
    int num = epoll_wait(m_epollfd, events, kMaxEvents, timeoutMs);
    if (num < 0) {
        LOG_ERROR("epoll_wait error: {}", strerror(errno));
        return;
    }

    LOG_DEBUG("epoll_wait returned {} events", num);

    for (int i = 0; i < num; i++) {
        Channel *channel = static_cast<Channel *>(events[i].data.ptr);
        channel->setRevents(events[i].events);

        LOG_DEBUG("Active fd={}, events={:#x}", channel->fd(), events[i].events);

        activeChannels.push_back(channel);
    }
}

void EpollPoller::updateChannel(Channel *channel)
{
    epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = channel->events();
    ev.data.ptr = channel;

    int fd = channel->fd();

    if (m_channels.count(fd) == 0) {
        if (epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
            LOG_ERROR("epoll_ctl ADD failed, fd={}, err={}", fd, strerror(errno));
            return;
        }
        m_channels[fd] = channel;
        LOG_DEBUG("epoll ADD fd={}, events={:#x}", fd, ev.events);
    } else {
        if (epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &ev) < 0) {
            LOG_ERROR("epoll_ctl MOD failed, fd={}, err={}", fd, strerror(errno));
            return;
        }
        LOG_DEBUG("epoll MOD fd={}, events={:#x}", fd, ev.events);
    }
}

void EpollPoller::removeChannel(Channel *channel)
{
    int fd = channel->fd();
    if (m_channels.count(fd)) {
        epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, nullptr);
        m_channels.erase(fd);
        LOG_DEBUG("epoll DEL fd={}", fd);
    }
}
