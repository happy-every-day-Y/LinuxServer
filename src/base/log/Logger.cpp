#include "Logger.h"
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

void Logger::init_minimal() {
    if (g_logger) return;

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    g_logger = std::make_shared<spdlog::logger>("CHAT", console_sink);
    g_logger->set_level(spdlog::level::info);
    g_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");
}

void Logger::init_full(const std::string& level_str) {
    spdlog::init_thread_pool(8192, 1);

    if (spdlog::get("CHAT")) {
        spdlog::drop("CHAT");
    }

    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/chat.log", 5 * 1024 * 1024, 3
    );

    spdlog::level::level_enum level = spdlog::level::debug;
    if (level_str == "info") level = spdlog::level::info;
    else if (level_str == "warn") level = spdlog::level::warn;
    else if (level_str == "error") level = spdlog::level::err;
    else if (level_str == "debug") level = spdlog::level::debug;

    g_logger = std::make_shared<spdlog::async_logger>(
        "CHAT",
        spdlog::sinks_init_list{console_sink, file_sink},
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block
    );
    g_logger->set_level(level);
    g_logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%^%l%$] %v");

    spdlog::register_logger(g_logger);
}

std::shared_ptr<spdlog::logger>& Logger::get() {
    return g_logger;
}
