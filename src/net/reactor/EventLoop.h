#pragma once
#include <vector>
#include <memory>

class Channel;
class EpollPoller;

class EventLoop{
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    bool m_quit;
    std::unique_ptr<EpollPoller> m_poller;
    std::vector<Channel*> m_activeChannels;
};