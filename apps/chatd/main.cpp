#include <iostream>
#include <memory>

#include "NetBootstrap.h"
#include "Logger.h"
#include "Config.h"
#include "MysqlPool.h"

int main() {
    Logger::init_minimal();
    LOG_INFO("Starting server...");

    Config::init(std::string(PROJECT_ROOT_DIR) + "/config.json");
    std::string log_level = Config::getString("logging.level", "debug");
    Logger::init_full(log_level);

    auto pool = MysqlPool::getConnectPool();
    pool->getConn();

    NetBootstrap net;
    net.start(Config::getInt("server.port", 9000));

    getchar();

    net.stop();
    return 0;
}