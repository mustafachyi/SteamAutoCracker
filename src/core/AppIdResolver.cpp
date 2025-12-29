#include "../../include/core/AppIdResolver.hpp"
#include "../../include/core/WebClient.hpp"
#include "../../include/core/Config.hpp"
#include <fstream>
#include <algorithm>
#include <charconv>
#include <nlohmann/json.hpp>

std::uint32_t AppIdResolver::ResolveFromFolder(const std::filesystem::path& folderPath) {
    auto appIdFile = folderPath / "steam_appid.txt";
    if (std::filesystem::exists(appIdFile)) {
        std::ifstream file(appIdFile);
        std::string line;
        if (std::getline(file, line)) {
            line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());
            std::uint32_t id = 0;
            if (IsNumeric(line) && std::from_chars(line.data(), line.data() + line.size(), id).ec == std::errc()) {
                return id;
            }
        }
    }
    return 0;
}

bool AppIdResolver::IsNumeric(std::string_view s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

std::vector<AppSearchResult> AppIdResolver::SearchByName(std::string_view name) {
    std::string rawJson = WebClient::Fetch(Config::Instance().apiUrl, L"/search?q=" + UrlEncode(name));
    std::vector<AppSearchResult> results;
    
    if (rawJson.empty()) return results;

    try {
        auto data = nlohmann::json::parse(rawJson, nullptr, false);
        if (!data.is_discarded() && data.is_array()) {
            results.reserve(data.size());
            for (const auto& item : data) {
                if (item.is_array() && item.size() >= 2) {
                    results.push_back({ item[0].get<uint32_t>(), item[1].get<std::string>(), true });
                }
            }
        }
    } catch (...) {}
    return results;
}

GameInfo AppIdResolver::GetGameInfo(uint32_t appId, StatusCallback cb) {
    GameInfo info = { appId, "", {} };
    std::string rawJson = WebClient::Fetch(Config::Instance().apiUrl, L"/lookup?id=" + std::to_wstring(appId));

    if (rawJson.empty()) return info;

    try {
        auto data = nlohmann::json::parse(rawJson, nullptr, false);
        if (!data.is_discarded() && data.is_array() && data.size() >= 2) {
            info.name = data[1].get<std::string>();
            if (data.size() >= 3 && data[2].is_array()) {
                info.dlcs.reserve(data[2].size());
                for (const auto& dlc : data[2]) {
                    if (dlc.is_array() && dlc.size() >= 2) {
                        info.dlcs.push_back({ dlc[0].get<uint32_t>(), dlc[1].get<std::string>(), true });
                    }
                }
            }
        }
    } catch (...) {}
    return info;
}

std::wstring AppIdResolver::UrlEncode(std::string_view value) {
    std::wstring escaped;
    escaped.reserve(value.size() * 3);
    const wchar_t hex[] = L"0123456789ABCDEF";
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped += static_cast<wchar_t>(c);
        } else if (c == ' ') {
            escaped += L'+';
        } else {
            escaped += L'%';
            escaped += hex[(c >> 4) & 0xF];
            escaped += hex[c & 0xF];
        }
    }
    return escaped;
}