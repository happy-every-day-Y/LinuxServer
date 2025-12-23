#pragma once

#include "MysqlConn.h"
#include "Config.h"

#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>

class MysqlPool {
public:
    static MysqlPool* getConnectPool();

    MysqlPool(const MysqlPool& obj) = delete;

    MysqlPool& operator=(const MysqlPool& obj) = delete;

    std::shared_ptr<MysqlConn> getConn();  // 消费者, 取出一个连接

    ~MysqlPool();
private:
    MysqlPool();

    void addConn();
    void produceConn();  // 生产者
    void recycleConn();

    std::string m_ip;
    std::string m_user;
    std::string m_passwd;
    std::string m_dbName;
    unsigned short m_port;
    size_t m_minSize;
    size_t m_maxSize;
    size_t m_timeout;
    size_t m_maxIdleTime;
    std::atomic<size_t> m_allAliveNum;     // 所有连接数量,包括队列中的以及被取出的
    std::atomic<bool> m_open;
    std::queue<MysqlConn*> m_connQueue;
    std::mutex m_mutexQ;
    std::condition_variable m_cv_producer, m_cv_consumer;
};