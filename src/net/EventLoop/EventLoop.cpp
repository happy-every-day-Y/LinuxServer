// EventLoop.cpp
#include "EventLoop.h"
#include "Logger.h"

EventLoop::EventLoop()
    : m_ioContext(),
      m_workGuard(boost::asio::make_work_guard(m_ioContext)) {
    LOG_INFO("Created");
}

EventLoop::~EventLoop() {
    LOG_INFO("Destroyed");
    stop();
    if(m_thread.joinable()) {
        LOG_INFO("Joining thread");
        m_thread.join();
    }
}

void EventLoop::run() {
    LOG_INFO("run() called, starting thread");
    m_thread = std::thread([this]{
        LOG_INFO("Thread started, running io_context");
        m_ioContext.run();
        LOG_INFO("io_context.run() exited");
    });
}

void EventLoop::stop() {
    LOG_INFO("stop() called");
    m_ioContext.stop();
}

void EventLoop::post(std::function<void()> cb) {
    LOG_DEBUG("post() called");
    boost::asio::post(m_ioContext, std::move(cb));
}

boost::asio::io_context &EventLoop::getIOContext()
{
    LOG_DEBUG("getIOContext() called");
    return m_ioContext;
}
