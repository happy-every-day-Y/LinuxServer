#include "MysqlPool.h"
#include "Logger.h"  // 添加日志头文件
#include <fstream>
#include <thread>

MysqlPool::MysqlPool() {
    LOG_INFO("Initializing MySQL connection pool...");
    
    // 从配置读取参数
    m_ip = Config::getString("database.host");
    m_user = Config::getString("database.user");
    m_passwd = Config::getString("database.password");
    m_dbName = Config::getString("database.dbname");
    m_port = Config::getInt("database.port");
    m_minSize = Config::getInt("database.minSize");
    m_maxSize = Config::getInt("database.maxSize");
    m_maxIdleTime = Config::getInt("database.maxIdleTime");
    m_timeout = Config::getInt("database.timeout");
    
    LOG_INFO("MySQL Pool Config - host: {}, user: {}, db: {}, port: {}", 
             m_ip, m_user, m_dbName, m_port);
    LOG_INFO("MySQL Pool Config - minSize: {}, maxSize: {}, timeout: {}ms, maxIdleTime: {}ms",
             m_minSize, m_maxSize, m_timeout, m_maxIdleTime);

    m_open = true;
    m_allAliveNum = 0;
    
    // 创建初始连接
    LOG_INFO("Creating initial {} connections...", m_minSize);
    for (size_t i = 0; i < m_minSize; ++i) {
        addConn();
    }
    LOG_INFO("Initial {} connections created successfully", m_minSize);
    
    // 启动生产和回收线程
    std::thread producer(&MysqlPool::produceConn, this);
    std::thread recycler(&MysqlPool::recycleConn, this);
    producer.detach();
    recycler.detach();
    
    LOG_INFO("MySQL connection pool initialized successfully");
}

MysqlPool::~MysqlPool() {
    LOG_INFO("Shutting down MySQL connection pool...");
    
    // 关闭生产和回收线程
    m_open = false;
    m_cv_producer.notify_all();
    
    LOG_INFO("Closing all database connections...");
    int remainingConnections = m_allAliveNum.load();
    
    // 关闭所有数据库连接
    while (m_allAliveNum > 0) {
        while (!m_connQueue.empty()) {
            auto conn = m_connQueue.front();
            m_connQueue.pop();
            --m_allAliveNum;
            delete conn;
            LOG_DEBUG("Connection closed, remaining: {}", m_allAliveNum.load());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    LOG_INFO("MySQL connection pool shutdown completed. Total connections closed: {}", 
             remainingConnections);
}

MysqlPool* MysqlPool::getConnectPool() {
    static MysqlPool pool;
    return &pool;
}

void MysqlPool::addConn() {
    try {
        MysqlConn* conn = new MysqlConn();
        LOG_DEBUG("Creating new MySQL connection...");
        
        if (!conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port)) {
            LOG_ERROR("Failed to create MySQL connection for pool");
            delete conn;
            return;
        }
        
        conn->refreshAliveTime();
        m_connQueue.push(conn);
        ++m_allAliveNum;
        
        LOG_DEBUG("New connection added to pool. Queue size: {}, Total alive: {}", 
                  m_connQueue.size(), m_allAliveNum.load());
    } catch (const std::exception& e) {
        LOG_ERROR("Exception while adding connection: {}", e.what());
    }
}

void MysqlPool::produceConn() {
    LOG_INFO("Connection producer thread started");
    
    while (m_open) {
        std::unique_lock<std::mutex> locker(m_mutexQ);
        
        // 队列中连接少于最少数量并且存活的所有连接数量小于最大数量时才增加连接
        m_cv_producer.wait(locker, [&] { 
            return (m_connQueue.size() < m_minSize && m_allAliveNum < m_maxSize) || !m_open; 
        });
        
        if (!m_open) {
            LOG_INFO("Connection producer thread exiting");
            break;
        }
        
        LOG_DEBUG("Producer: Queue size {}, Total alive {}, need more connections", 
                  m_connQueue.size(), m_allAliveNum.load());
        
        addConn();
        m_cv_consumer.notify_all();
    }
    
    LOG_INFO("Connection producer thread stopped");
}

void MysqlPool::recycleConn() {
    LOG_INFO("Connection recycler thread started");
    
    while (m_open) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        if (!m_open) break;
        
        std::unique_lock<std::mutex> locker(m_mutexQ);
        
        size_t recycled = 0;
        while (m_connQueue.size() > m_minSize) {
            MysqlConn* conn = m_connQueue.front();
            size_t idleTime = conn->getAliveTime();
            
            if (idleTime >= m_maxIdleTime) {
                m_connQueue.pop();
                --m_allAliveNum;
                delete conn;
                ++recycled;
                LOG_DEBUG("Recycled idle connection (idle for {}ms)", idleTime);
            } else {
                break;
            }
        }
        
        if (recycled > 0) {
            LOG_INFO("Recycled {} idle connections. Queue size: {}, Total alive: {}", 
                     recycled, m_connQueue.size(), m_allAliveNum.load());
        }
    }
    
    LOG_INFO("Connection recycler thread stopped");
}

std::shared_ptr<MysqlConn> MysqlPool::getConn() {
    LOG_DEBUG("Requesting connection from pool...");
    
    std::unique_lock<std::mutex> locker(m_mutexQ);
    auto startTime = std::chrono::steady_clock::now();
    
    // 等待连接可用
    while (m_connQueue.empty()) {
        LOG_DEBUG("Connection pool empty, waiting for available connection...");
        
        if (std::cv_status::timeout == m_cv_consumer.wait_for(locker, 
                std::chrono::milliseconds(m_timeout))) {
            
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - startTime).count();
            
            if (m_connQueue.empty()) {
                LOG_WARN("Timeout waiting for connection after {}ms (timeout: {}ms)", 
                         elapsed, m_timeout);
                // 可以返回nullptr或者继续等待
                continue;
            }
        }
    }
    
    // 获取连接
    MysqlConn* rawConn = m_connQueue.front();
    m_connQueue.pop();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();
    
    LOG_DEBUG("Connection obtained after {}ms. Queue size: {}, Total alive: {}", 
              elapsed, m_connQueue.size(), m_allAliveNum.load());
    
    // 指定共享指针删除器为归还数据库连接
    std::shared_ptr<MysqlConn> connptr(rawConn, [this](MysqlConn* conn) {
        LOG_DEBUG("Returning connection to pool...");
        std::lock_guard<std::mutex> locker(m_mutexQ);
        conn->refreshAliveTime();
        m_connQueue.push(conn);
        LOG_DEBUG("Connection returned. Queue size: {}, Total alive: {}", 
                  m_connQueue.size(), m_allAliveNum.load());
        m_cv_consumer.notify_one();  // 通知等待的消费者
    });
    
    m_cv_producer.notify_all();
    LOG_DEBUG("Connection ready for use");
    
    return connptr;
}