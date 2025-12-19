#include "Config.h"
#include "Logger.h"
#include <fstream>

nlohmann::json Config::configJson;
std::unordered_map<std::string, std::string> Config::flatMap;

void Config::init(const std::string& filename) {
    LOG_INFO("Initializing configuration from '{}'", trimFilePath(filename.c_str()));

    std::ifstream f(filename);
    if (!f.is_open()) {
        LOG_ERROR("Cannot open config file: '{}'", trimFilePath(filename.c_str()));
        return;
    }

    try {
        f >> configJson;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to parse config file '{}': {}", trimFilePath(filename.c_str()), e.what());
        return;
    }

    flatten(configJson);

    LOG_INFO("Configuration loaded successfully from '{}'", trimFilePath(filename.c_str()));
    LOG_INFO("Flattened keys:");
    for (const auto& kv : flatMap) {
        LOG_INFO("  {} = {}", kv.first, kv.second);
    }
}

void Config::flatten(const nlohmann::json& j, const std::string& prefix) {
    for (auto it = j.begin(); it != j.end(); ++it) {
        std::string key = prefix.empty() ? it.key() : prefix + "." + it.key();
        if (it->is_object()) {
            flatten(*it, key);
        } else {
            flatMap[key] = it->dump();
            if (it->is_string()) flatMap[key] = it->get<std::string>();
        }
    }
}

int Config::getInt(const std::string& key, int defaultValue) {
    if (flatMap.count(key))
        return std::stoi(flatMap[key]);
    return defaultValue;
}

std::string Config::getString(const std::string& key, const std::string& defaultValue) {
    if (flatMap.count(key))
        return flatMap[key];
    return defaultValue;
}

bool Config::getBool(const std::string& key, bool defaultValue) {
    if (flatMap.count(key))
        return flatMap[key] == "true" || flatMap[key] == "1";
    return defaultValue;
}
