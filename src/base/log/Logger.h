#pragma once

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR "."
#endif

#include <memory>
#include <spdlog/spdlog.h>
#include <string>

class Logger {
public:
    static void init_minimal();
    static void init_full(const std::string& level_str);
    static std::shared_ptr<spdlog::logger>& get();

private:
    static inline std::shared_ptr<spdlog::logger> g_logger = nullptr;
};

inline std::string trimFilePath(const char* fullPath) {
    std::string path(fullPath);
    std::string root(PROJECT_ROOT_DIR);
    if (path.find(root) == 0)
        path = path.substr(root.size() + 1);
    return path;
}

#define LOG_INFO(...)  Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::info, __VA_ARGS__)
#define LOG_WARN(...)  Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::warn, __VA_ARGS__)
#define LOG_ERROR(...) Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::err, __VA_ARGS__)
#define LOG_DEBUG(...) Logger::get()->log(spdlog::source_loc{__FILE__, __LINE__, __FUNCTION__}, spdlog::level::debug, __VA_ARGS__)