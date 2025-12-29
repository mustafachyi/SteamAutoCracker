#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <filesystem>
#include <windows.h>

class Config {
public:
    std::wstring apiUrl;
    std::wstring lastFolder;

    static Config& Instance() {
        static Config instance;
        return instance;
    }

    void Load() {
        wchar_t buf[2048];
        std::filesystem::path path = std::filesystem::current_path() / "config.ini";
        
        GetPrivateProfileStringW(L"Settings", L"ApiUrl", L"steam.selfhoster.nl", buf, 2048, path.c_str());
        std::wstring rawUrl = buf;
        
        const std::wstring proto = L"://";
        size_t protoPos = rawUrl.find(proto);
        if (protoPos != std::wstring::npos) {
            rawUrl = rawUrl.substr(protoPos + proto.length());
        }
        
        size_t slashPos = rawUrl.find(L'/');
        if (slashPos != std::wstring::npos) {
            rawUrl = rawUrl.substr(0, slashPos);
        }
        
        apiUrl = rawUrl;

        GetPrivateProfileStringW(L"Settings", L"LastFolder", L"", buf, 2048, path.c_str());
        lastFolder = buf;
    }

    void Save() {
        std::filesystem::path path = std::filesystem::current_path() / "config.ini";
        WritePrivateProfileStringW(L"Settings", L"ApiUrl", apiUrl.c_str(), path.c_str());
        WritePrivateProfileStringW(L"Settings", L"LastFolder", lastFolder.c_str(), path.c_str());
    }

private:
    Config() { Load(); }
};

#endif