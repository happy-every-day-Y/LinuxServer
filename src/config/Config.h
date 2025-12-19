#pragma once
#include <string>
#include <nlohmann/json.hpp>
#include <unordered_map>

class Config {
public:
    static void init(const std::string& filename);
    
    static int getInt(const std::string& key, int defaultValue = 0);
    static std::string getString(const std::string& key, const std::string& defaultValue = "");
    static bool getBool(const std::string& key, bool defaultValue = false);
    
private:
    static nlohmann::json configJson;
    static std::unordered_map<std::string, std::string> flatMap;
    
    static void flatten(const nlohmann::json& j, const std::string& prefix = "");
};
