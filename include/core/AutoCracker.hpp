#ifndef AUTO_CRACKER_HPP
#define AUTO_CRACKER_HPP

#include "AppIdResolver.hpp"
#include <filesystem>
#include <functional>
#include <string>
#include <vector>
#include <windows.h>

class AutoCracker {
public:
    struct Settings {
        std::filesystem::path gameFolder;
        std::string selectedCrack; 
        bool enableSteamless;
    };

    using StatusCallback = std::function<void(const std::string&)>;

    static bool Apply(const GameInfo& info, const Settings& settings, StatusCallback cb);
    static bool Revert(const Settings& settings, StatusCallback cb);
    static bool ValidateTools(std::string& outError);
    static bool IsCracked(const std::filesystem::path& gameFolder);

private:
    struct BackupNames {
        std::string x86;
        std::string x64;
    };

    static void RunSteamless(const std::filesystem::path& exePath, const std::filesystem::path& base, StatusCallback cb);
    static void ApplyEmulator(const std::filesystem::path& targetDir, const GameInfo& info, const std::filesystem::path& emuSource, const BackupNames& backups, const std::filesystem::path& base, StatusCallback cb);
    static void ProcessConfigFile(const std::filesystem::path& path, const GameInfo& info, const std::string& apiVersion);
    static std::string GetFileVersion(const std::filesystem::path& path);
    static bool RunProcess(const std::wstring& cmd, const std::filesystem::path& workDir = "");
    static BackupNames LoadBackupConfig(const std::filesystem::path& crackDir);
};

#endif