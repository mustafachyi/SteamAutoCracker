#include "../../include/core/AppIdResolver.hpp"
#include "../../include/core/WebClient.hpp"
#include "../../include/core/Config.hpp"
#include <fstream>
#include <algorithm>
#include <charconv>

namespace {
    struct JsonCursor {
        const char* ptr;
        const char* end;

        void Skip() {
            while (ptr < end && (unsigned char)*ptr <= 32) ptr++;
        }

        bool Match(char c) {
            Skip();
            if (ptr < end && *ptr == c) { ptr++; return true; }
            return false;
        }

        bool Peek(char c) {
            Skip();
            return ptr < end && *ptr == c;
        }

        std::uint32_t ParseUint() {
            Skip();
            std::uint32_t v = 0;
            while (ptr < end && *ptr >= '0' && *ptr <= '9') {
                v = v * 10 + (*ptr++ - '0');
            }
            return v;
        }

        std::string ParseString() {
            Skip();
            std::string res;
            if (ptr >= end || *ptr != '"') return res;
            ptr++;
            while (ptr < end && *ptr != '"') {
                if (*ptr == '\\') {
                    ptr++;
                    if (ptr >= end) break;
                    char c = *ptr;
                    if (c == 'n') res += '\n';
                    else if (c == 'r') res += '\r';
                    else if (c == 't') res += '\t';
                    else if (c == 'u') {
                        if (ptr + 4 < end) ptr += 4;
                    } else {
                        res += c;
                    }
                } else {
                    res += *ptr;
                }
                ptr++;
            }
            if (ptr < end) ptr++;
            return res;
        }
    };
}

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

    JsonCursor c{rawJson.data(), rawJson.data() + rawJson.size()};
    if (!c.Match('[')) return results;

    while (c.ptr < c.end && !c.Peek(']')) {
        if (c.Match('[')) {
            uint32_t id = c.ParseUint();
            if (c.Match(',')) {
                std::string appName = c.ParseString();
                results.push_back({id, appName, true});
            }
            while (c.ptr < c.end && *c.ptr != ']') c.ptr++;
            c.Match(']');
        }
        c.Match(',');
    }

    return results;
}

GameInfo AppIdResolver::GetGameInfo(uint32_t appId, StatusCallback) {
    GameInfo info = { appId, "", {} };
    std::string rawJson = WebClient::Fetch(Config::Instance().apiUrl, L"/lookup?id=" + std::to_wstring(appId));
    if (rawJson.empty()) return info;

    JsonCursor c{rawJson.data(), rawJson.data() + rawJson.size()};
    
    if (!c.Match('[')) return info;
    c.ParseUint();
    if (!c.Match(',')) return info;
    
    info.name = c.ParseString();

    if (c.Match(',')) {
        if (c.Match('[')) {
            if (c.Peek('[')) {
                while (c.Match('[')) {
                    uint32_t dlcId = c.ParseUint();
                    if (c.Match(',')) {
                        std::string dlcName = c.ParseString();
                        info.dlcs.push_back({dlcId, dlcName, true});
                    }
                    while (c.ptr < c.end && *c.ptr != ']') c.ptr++;
                    c.Match(']');
                    c.Match(',');
                }
            }
        }
    }
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