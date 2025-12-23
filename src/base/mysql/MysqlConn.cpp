#include "MysqlConn.h"
#include "Logger.h"

MysqlConn::MysqlConn() {
    // 初始化
    m_conn = mysql_init(nullptr);
    if (m_conn == nullptr) {
        LOG_ERROR("mysql_init failed: {}", mysql_error(nullptr));
        return;
    }
    
    // 设置接口编码
    if (mysql_set_character_set(m_conn, "utf8") != 0) {
        LOG_WARN("Failed to set character set to utf8: {}", mysql_error(m_conn));
    } else {
        LOG_DEBUG("MySQL connection initialized with UTF-8 charset");
    }
}

MysqlConn::~MysqlConn() {
    if (m_conn != nullptr) {
        LOG_DEBUG("Closing MySQL connection");
        mysql_close(m_conn);
    }
    freeResult();
}

bool MysqlConn::connect(std::string user, std::string passwd, std::string dbName, std::string ip, unsigned short port) {
    LOG_DEBUG("Attempting to connect to MySQL: {}@{}:{}/{}", user, ip, port, dbName);
    
    auto ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), 
                                  dbName.c_str(), port, nullptr, 0);
    
    if (ptr == nullptr) {
        LOG_ERROR("MySQL connection failed: {}", mysql_error(m_conn));
        return false;
    }
    
    LOG_INFO("MySQL connected successfully: {}@{}:{}/{}", user, ip, port, dbName);
    return true;
}

bool MysqlConn::update(std::string sql) {
    LOG_DEBUG("Executing update SQL: {}", sql);
    
    if (mysql_query(m_conn, sql.c_str())) {
        LOG_ERROR("MySQL update failed: {} - SQL: {}", mysql_error(m_conn), sql);
        return false;
    }
    
    my_ulonglong affectedRows = mysql_affected_rows(m_conn);
    LOG_DEBUG("Update successful, affected rows: {}", affectedRows);
    return true;
}

bool MysqlConn::query(std::string sql) {
    LOG_DEBUG("Executing query SQL: {}", sql);
    
    freeResult();
    
    if (mysql_query(m_conn, sql.c_str())) {
        LOG_ERROR("MySQL query failed: {} - SQL: {}", mysql_error(m_conn), sql);
        return false;
    }
    
    if (!m_conn) {
        LOG_ERROR("MySQL connection is null");
        return false;
    }
    
    m_result = mysql_store_result(m_conn);
    if (m_result == nullptr) {
        // 可能是没有结果集或错误
        if (mysql_field_count(m_conn) == 0) {
            // 查询没有返回结果集（如UPDATE等）
            LOG_DEBUG("Query executed successfully, no result set returned");
            return true;
        } else {
            LOG_ERROR("Failed to store result: {}", mysql_error(m_conn));
            return false;
        }
    }
    
    my_ulonglong numRows = mysql_num_rows(m_result);
    unsigned int numFields = mysql_num_fields(m_result);
    LOG_DEBUG("Query successful, rows: {}, fields: {}", numRows, numFields);
    
    return true;
}

bool MysqlConn::next() {
    if (m_result != nullptr) {
        m_row = mysql_fetch_row(m_result);
        bool hasNext = (m_row != nullptr);
        if (!hasNext) {
            LOG_DEBUG("No more rows in result set");
        }
        return hasNext;
    }
    LOG_WARN("next() called but result set is null");
    return false;
}

std::string MysqlConn::value(int index) {
    if (m_result == nullptr) {
        LOG_WARN("value() called but result set is null");
        return std::string();
    }
    
    int colCount = mysql_num_fields(m_result);
    if (index >= colCount || index < 0) {
        LOG_WARN("Column index out of range: {} (total columns: {})", index, colCount);
        return std::string();
    }
    
    if (m_row == nullptr) {
        LOG_WARN("value() called but current row is null");
        return std::string();
    }
    
    char* val = m_row[index];
    if (val == nullptr) {
        LOG_DEBUG("Column {} value is NULL", index);
        return std::string();
    }
    
    unsigned long length = mysql_fetch_lengths(m_result)[index];
    LOG_DEBUG("Retrieved column {} value, length: {}", index, length);
    return std::string(val, length);
}

bool MysqlConn::transaction() {
    LOG_DEBUG("Starting transaction");
    if (mysql_autocommit(m_conn, false) != 0) {
        LOG_ERROR("Failed to start transaction: {}", mysql_error(m_conn));
        return false;
    }
    LOG_DEBUG("Transaction started");
    return true;
}

bool MysqlConn::commit() {
    LOG_DEBUG("Committing transaction");
    if (mysql_commit(m_conn) != 0) {
        LOG_ERROR("Failed to commit transaction: {}", mysql_error(m_conn));
        return false;
    }
    LOG_DEBUG("Transaction committed");
    return true;
}

bool MysqlConn::rollback() {
    LOG_DEBUG("Rolling back transaction");
    if (mysql_rollback(m_conn) != 0) {
        LOG_ERROR("Failed to rollback transaction: {}", mysql_error(m_conn));
        return false;
    }
    LOG_DEBUG("Transaction rolled back");
    return true;
}

void MysqlConn::refreshAliveTime() {
    m_aliveTime = std::chrono::steady_clock::now();
    LOG_DEBUG("Connection alive time refreshed");
}

size_t MysqlConn::getAliveTime() {
    std::chrono::nanoseconds res = std::chrono::steady_clock::now() - m_aliveTime;
    std::chrono::milliseconds millsec = std::chrono::duration_cast<std::chrono::milliseconds>(res);
    size_t time = millsec.count();
    LOG_DEBUG("Connection alive time: {} ms", time);
    return time;
}

void MysqlConn::freeResult() {
    if (m_result) {
        LOG_DEBUG("Freeing MySQL result set");
        mysql_free_result(m_result);
        m_result = nullptr;
        m_row = nullptr;
    }
}