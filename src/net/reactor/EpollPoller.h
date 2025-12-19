#pragma once
#include <vector>
#include <unordered_map>

class Channel;

class EpollPoller{
public:
    EpollPoller();
    ~EpollPoller();
    
    void poll(int timeoutMs, std::vector<Channel*>& activeChannels);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);

private:
    int m_epollfd;
    std::unordered_map<int, Channel*> m_channels;
};