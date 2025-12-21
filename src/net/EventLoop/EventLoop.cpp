// EventLoop.cpp
#include "EventLoop.h"

EventLoop::EventLoop()
    : m_ioContext(),
      m_workGuard(boost::asio::make_work_guard(m_ioContext)) {}

EventLoop::~EventLoop() {
    stop();
    if(m_thread.joinable()) m_thread.join();
}

void EventLoop::run() {
    m_thread = std::thread([this]{
        m_ioContext.run();
    });
}

void EventLoop::stop() {
    m_ioContext.stop();
}

void EventLoop::post(std::function<void()> cb) {
    boost::asio::post(m_ioContext, std::move(cb));
}

boost::asio::io_context &EventLoop::getIOContext()
{
     return m_ioContext;
}
