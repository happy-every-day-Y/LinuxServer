// ThreadPool.h
#pragma once
#include <future>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include "BS_thread_pool.hpp"
#include "Logger.h"

class ThreadPool {
public:
    // 初始化线程池
    static void init(size_t threads = 0) {
        if (!pool_) {
            size_t n = threads == 0 ? std::thread::hardware_concurrency() : threads;
            pool_ = std::make_unique<BS::thread_pool<>>(n);
            LOG_INFO("ThreadPool initialized with {} threads", n);
        }
    }

    // 提交有返回值任务
    template<typename Func, typename... Args>
    static auto submit_task(Func&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        if (!pool_) init();
        try {
            return pool_->submit_task(std::forward<Func>(f), std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            LOG_ERROR("ThreadPool submit error: {}", e.what());
            throw;
        }
    }

    // 提交无返回值任务
    template<typename Func, typename... Args>
    static void detach_task(Func&& f, Args&&... args) {
        if (!pool_) init();
        try {
            pool_->detach_task(std::forward<Func>(f), std::forward<Args>(args)...);
        } catch (const std::exception& e) {
            LOG_ERROR("ThreadPool detach error: {}", e.what());
            throw;
        }
    }

    // 等待所有任务完成
    static void wait() {
        if (pool_) pool_->wait();
    }

    // 获取当前线程池状态
    static size_t get_tasks_queued() {
        return pool_ ? pool_->get_tasks_queued() : 0;
    }

    static size_t get_tasks_running() {
        return pool_ ? pool_->get_tasks_running() : 0;
    }

    static size_t get_tasks_total() {
        return pool_ ? pool_->get_tasks_total() : 0;
    }

    static size_t get_thread_count() {
        return pool_ ? pool_->get_thread_count() : 0;
    }

private:
    ThreadPool() = default;
    ~ThreadPool() = default;

    static inline std::unique_ptr<BS::thread_pool<>> pool_ = nullptr;
};
