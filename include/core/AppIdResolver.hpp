#ifndef APP_ID_RESOLVER_HPP
#define APP_ID_RESOLVER_HPP

#include <filesystem>
#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <functional>
#include <cstdint>

struct AppSearchResult {
    std::uint32_t id;
    std::string name;
    bool isWindowsCompatible;
};

struct GameInfo {
    uint32_t id;
    std::string name;
    std::vector<AppSearchResult> dlcs;
};

class AppIdResolver {
public:
    using StatusCallback = std::function<void(const std::string&)>;

    std::uint32_t ResolveFromFolder(const std::filesystem::path& folderPath);
    std::vector<AppSearchResult> SearchByName(std::string_view name);
    GameInfo GetGameInfo(uint32_t appId, StatusCallback cb = nullptr);

private:
    bool IsNumeric(std::string_view s);
    std::wstring UrlEncode(std::string_view value);
};

#endif