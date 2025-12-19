#include "EventLoop.h"
#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"

EventLoop::EventLoop()
: m_quit(false), m_poller(new EpollPoller())
{
    LOG_INFO("EventLoop created");
}

EventLoop::~EventLoop()
{
    LOG_INFO("EventLoop destroyed");
}
void EventLoop::loop()
{
    LOG_INFO("EventLoop loop started");

    while(!m_quit){
        m_activeChannels.clear();

        LOG_DEBUG("EventLoop polling...");
        m_poller->poll(1000, m_activeChannels);

        LOG_DEBUG("EventLoop got {} active channels", m_activeChannels.size());

        for(Channel* ch : m_activeChannels){
            LOG_DEBUG("EventLoop handling fd={}", ch->fd());
            ch->handleEvent();
        }
    }

    LOG_INFO("EventLoop loop exited");
}

void EventLoop::updateChannel(Channel *channel)
{
    LOG_DEBUG("EventLoop updateChannel fd={}", channel->fd());
    m_poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    LOG_DEBUG("EventLoop removeChannel fd={}", channel->fd());
    m_poller->removeChannel(channel);
}
