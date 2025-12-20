#pragma once
#include <functional>
#include <cstdint>
#include <sys/socket.h>
#include <unistd.h>

class EventLoop;

class Channel{
public:
    using EventCallback = std::function<void()>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent();

    void setReadCallback(EventCallback cb);
    void setWriteCallback(EventCallback cb);
    
    int fd() const;
    uint32_t events() const;
    void setRevents(uint32_t revt);

    void enableReading();
    void enableWriting();
    void disableReading();
    void disableWriting();
    void disableAll();

private:
    EventLoop* m_loop;
    const int m_fd;

    uint32_t m_events;
    uint32_t m_revents;

    EventCallback m_readCallback;
    EventCallback m_writeCallback;
};