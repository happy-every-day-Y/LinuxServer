// EventLoop.h
#pragma once
#include <boost/asio.hpp>
#include <thread>
#include <memory>
#include <functional>

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void run();                         // 启动事件循环
    void stop();                        // 停止事件循环
    void post(std::function<void()> cb); // 投递任务到 io_context
    boost::asio::io_context& getIOContext();

private:
    boost::asio::io_context m_ioContext;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_workGuard;
    std::thread m_thread;
};
