#include "../../include/core/AutoCracker.hpp"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>
#include <vector>

static bool EndsWith(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() && str.substr(str.size() - suffix.size()) == suffix;
}

static bool CheckFileStructure(const std::filesystem::path& base, const std::vector<std::string>& files) {
    for (const auto& f : files) {
        if (!std::filesystem::exists(base / f)) return false;
    }
    return true;
}

static void LogAction(const AutoCracker::StatusCallback& cb, const char* tag, const std::filesystem::path& file, const std::filesystem::path& base) {
    if (!cb) return;
    std::error_code ec;
    auto rel = std::filesystem::relative(file, base, ec);
    cb(std::string("[") + tag + "] " + (ec ? file.filename().string() : rel.string()));
}

bool AutoCracker::ValidateTools(std::string& outError) {
    const auto tools = std::filesystem::current_path() / "tools";
    if (!std::filesystem::exists(tools)) {
        outError = "Tools directory missing.";
        return false;
    }

    const std::vector<std::string> slFiles = {
        "Steamless.CLI.exe", "Steamless.CLI.exe.config",
        "Plugins/ExamplePlugin.dll", "Plugins/SharpDisasm.dll", "Plugins/Steamless.API.dll",
        "Plugins/Steamless.Unpacker.Variant10.x86.dll", "Plugins/Steamless.Unpacker.Variant20.x86.dll",
        "Plugins/Steamless.Unpacker.Variant21.x86.dll", "Plugins/Steamless.Unpacker.Variant30.x64.dll",
        "Plugins/Steamless.Unpacker.Variant30.x86.dll", "Plugins/Steamless.Unpacker.Variant31.x64.dll",
        "Plugins/Steamless.Unpacker.Variant31.x86.dll"
    };

    if (!CheckFileStructure(tools / "Steamless", slFiles)) {
        outError = "Steamless validation failed.";
        return false;
    }

    const std::vector<std::string> emuFiles = {
        "dlc_creamapi/config_override.ini", "dlc_creamapi/files/cream_api.ini",
        "dlc_creamapi/files/steam_api.dll", "dlc_creamapi/files/steam_api64.dll",
        "game_ali213/files/SteamConfig.ini", "game_ali213/files/steam_api.dll",
        "game_ali213/files/steam_api64.dll", "game_goldberg/files/steam_api.dll",
        "game_goldberg/files/steam_api64.dll", "game_goldberg/files/steam_settings/DLC.txt",
        "game_goldberg/files/steam_settings/steam_appid.txt"
    };

    if (!CheckFileStructure(tools / "sac_emu", emuFiles)) {
        outError = "Emulator validation failed.";
        return false;
    }

    return true;
}

bool AutoCracker::IsCracked(const std::filesystem::path& gameFolder) {
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(gameFolder)) {
            auto name = entry.path().filename().string();
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (entry.is_regular_file()) {
                if (name == "steam_api.dll.bak" || name == "steam_api64.dll.bak" || 
                    name == "cream_api.ini" || name == "steamconfig.ini") return true;
            } else if (entry.is_directory()) {
                if (name == "steam_settings" || name == "profile") return true;
            }
        }
    } catch (...) {}
    return false;
}

bool AutoCracker::Apply(const GameInfo& info, const Settings& settings, StatusCallback cb) {
    const auto crackDir = std::filesystem::current_path() / "tools" / "sac_emu" / settings.selectedCrack;
    const auto emuBase = crackDir / "files";
    const auto backups = LoadBackupConfig(crackDir);

    if (cb) cb("Scanning game directory...");

    std::vector<std::filesystem::path> dllLocations;
    std::vector<std::filesystem::path> executables;

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(settings.gameFolder, std::filesystem::directory_options::skip_permission_denied)) {
            if (!entry.is_regular_file()) continue;
            auto path = entry.path();
            auto lowerName = path.filename().string();
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            if (lowerName == "steam_api.dll" || lowerName == "steam_api64.dll") {
                LogAction(cb, "SCAN  ", path, settings.gameFolder);
                auto parent = path.parent_path();
                bool found = false;
                for(const auto& loc : dllLocations) if(loc == parent) { found = true; break; }
                if(!found) dllLocations.push_back(std::move(parent));
            }
            else if (settings.enableSteamless && EndsWith(lowerName, ".exe")) {
                executables.push_back(std::move(path));
            }
        }
    } catch (...) {}

    if (settings.enableSteamless) {
        for (const auto& exe : executables) RunSteamless(exe, settings.gameFolder, cb);
    }

    if (dllLocations.empty()) {
        if (cb) cb("No Steam API found.");
        return false;
    }

    for (const auto& dir : dllLocations) ApplyEmulator(dir, info, emuBase, backups, settings.gameFolder, cb);

    if (cb) cb("Success.");
    return true;
}

bool AutoCracker::Revert(const Settings& settings, StatusCallback cb) {
    const auto crackDir = std::filesystem::current_path() / "tools" / "sac_emu" / settings.selectedCrack;
    const auto backups = LoadBackupConfig(crackDir);
    const std::set<std::string> artifacts = { "steam_appid.txt", "steamconfig.ini", "cream_api.ini" };
    
    std::vector<std::filesystem::path> toDelete;
    std::vector<std::pair<std::filesystem::path, std::filesystem::path>> toRestore;

    if (cb) cb("Analyzing file system...");

    try {
        for (auto it = std::filesystem::recursive_directory_iterator(settings.gameFolder, std::filesystem::directory_options::skip_permission_denied); 
             it != std::filesystem::recursive_directory_iterator(); ++it) {
            
            auto lowerName = it->path().filename().string();
            std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);

            if (it->is_directory()) {
                if (lowerName == "steam_settings" || lowerName == "profile") {
                    toDelete.push_back(it->path());
                    it.disable_recursion_pending();
                }
            } else {
                if (artifacts.count(lowerName)) toDelete.push_back(it->path());
                else if (lowerName == "steam_api.dll.bak" || lowerName == "steam_api64.dll.bak" || 
                         lowerName == backups.x86 || lowerName == backups.x64 || EndsWith(lowerName, ".exe.bak")) {
                    
                    auto bak = it->path();
                    auto orig = bak;
                    orig.replace_extension("");
                    if (bak.extension() == ".bak" && orig.extension() != ".dll" && !EndsWith(lowerName, ".exe.bak")) {
                         orig = bak.parent_path() / bak.stem(); 
                    }
                    if (lowerName == backups.x86) orig = bak.parent_path() / "steam_api.dll";
                    else if (lowerName == backups.x64) orig = bak.parent_path() / "steam_api64.dll";

                    toRestore.push_back({std::move(bak), std::move(orig)});
                }
            }
        }

        for (const auto& p : toDelete) {
            LogAction(cb, "CLEAN ", p, settings.gameFolder);
            std::filesystem::remove_all(p);
        }
        for (const auto& [bak, orig] : toRestore) {
            LogAction(cb, "REVERT", orig, settings.gameFolder);
            if (std::filesystem::exists(orig)) std::filesystem::remove(orig);
            std::filesystem::rename(bak, orig);
        }
    } catch (...) { return false; }

    if (cb) cb("Reverted.");
    return true;
}

AutoCracker::BackupNames AutoCracker::LoadBackupConfig(const std::filesystem::path& crackDir) {
    BackupNames names = { "steam_api.dll.bak", "steam_api64.dll.bak" };
    auto cfgPath = crackDir / "config_override.ini";
    if (std::filesystem::exists(cfgPath)) {
        std::ifstream file(cfgPath);
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == ';' || line[0] == '[') continue;
            if (auto sep = line.find('='); sep != std::string::npos) {
                auto key = line.substr(0, sep), val = line.substr(sep + 1);
                auto trim = [](std::string& s) {
                    s.erase(0, s.find_first_not_of(" \t\r\n"));
                    s.erase(s.find_last_not_of(" \t\r\n") + 1);
                };
                trim(key); trim(val);
                std::transform(key.begin(), key.end(), key.begin(), ::tolower);
                if (key == "steamapi") names.x86 = val;
                else if (key == "steamapi64") names.x64 = val;
            }
        }
    }
    return names;
}

void AutoCracker::RunSteamless(const std::filesystem::path& exePath, const std::filesystem::path& base, StatusCallback cb) {
    const auto slDir = std::filesystem::current_path() / "tools" / "Steamless";
    LogAction(cb, "PATCH ", exePath, base);
    
    std::wstring cmd = L"\"" + (slDir / "Steamless.CLI.exe").wstring() + L"\" \"" + exePath.wstring() + L"\"";
    if (RunProcess(cmd, slDir)) {
        auto unpacked = exePath; unpacked += ".unpacked.exe";
        if (std::filesystem::exists(unpacked)) {
            auto backup = exePath; backup += ".bak";
            LogAction(cb, "BACKUP", backup, base);
            if (std::filesystem::exists(backup)) std::filesystem::remove(backup);
            std::filesystem::rename(exePath, backup);
            
            LogAction(cb, "WRITE ", exePath, base);
            std::filesystem::rename(unpacked, exePath);
        }
    }
}

void AutoCracker::ApplyEmulator(const std::filesystem::path& targetDir, const GameInfo& info, const std::filesystem::path& emuSource, const BackupNames& backups, const std::filesystem::path& base, StatusCallback cb) {
    std::string apiVersion = "0.0.0.0";
    auto origApi = targetDir / "steam_api64.dll";
    if (!std::filesystem::exists(origApi)) origApi = targetDir / "steam_api.dll";
    if (std::filesystem::exists(origApi)) apiVersion = GetFileVersion(origApi);

    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(emuSource)) {
            auto relPath = std::filesystem::relative(entry.path(), emuSource);
            auto destPath = targetDir / relPath;
            
            if (entry.is_directory()) {
                std::filesystem::create_directories(destPath);
            } else {
                auto lowerName = entry.path().filename().string();
                std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
                
                if (std::filesystem::exists(destPath)) {
                    std::filesystem::path bak;
                    if (lowerName == "steam_api.dll") bak = destPath.parent_path() / backups.x86;
                    else if (lowerName == "steam_api64.dll") bak = destPath.parent_path() / backups.x64;
                    
                    if (!bak.empty()) {
                        LogAction(cb, "BACKUP", bak, base);
                        if (!std::filesystem::exists(bak)) std::filesystem::rename(destPath, bak);
                        else std::filesystem::remove(destPath);
                    }
                }
                
                LogAction(cb, EndsWith(lowerName, ".txt") || EndsWith(lowerName, ".ini") ? "CONFIG" : "WRITE ", destPath, base);
                std::filesystem::copy_file(entry.path(), destPath, std::filesystem::copy_options::overwrite_existing);
                
                if (EndsWith(lowerName, ".txt") || EndsWith(lowerName, ".ini") || EndsWith(lowerName, ".cfg")) {
                    ProcessConfigFile(destPath, info, apiVersion);
                }
            }
        }
    } catch (...) {}
}

void AutoCracker::ProcessConfigFile(const std::filesystem::path& path, const GameInfo& info, const std::string& apiVersion) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in.is_open()) return;
    auto size = in.tellg();
    if (size <= 0) return;
    std::string content(size, '\0');
    in.seekg(0);
    in.read(&content[0], size);
    in.close();

    auto Replace = [&](const std::string& from, const std::string& to) {
        if (from.empty()) return;
        size_t pos = 0;
        while ((pos = content.find(from, pos)) != std::string::npos) {
            content.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    Replace("SAC_AppID", std::to_string(info.id));
    Replace("SAC_APIVersion", apiVersion);
    
    std::string dlcBlock, noSpaceDlcBlock;
    size_t reserveSize = info.dlcs.size() * 64;
    dlcBlock.reserve(reserveSize);
    noSpaceDlcBlock.reserve(reserveSize);
    
    for (const auto& dlc : info.dlcs) {
        std::string idStr = std::to_string(dlc.id);
        dlcBlock += idStr + " = " + dlc.name + "\r\n";
        noSpaceDlcBlock += idStr + "=" + dlc.name + "\r\n";
    }
    
    Replace("SAC_DLC", dlcBlock);
    Replace("SAC_NoSpaceDLC", noSpaceDlcBlock);

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (out.is_open()) out.write(content.data(), content.size());
}

std::string AutoCracker::GetFileVersion(const std::filesystem::path& path) {
    DWORD dummy, size = GetFileVersionInfoSizeW(path.c_str(), &dummy);
    if (size == 0) return "0.0.0.0";
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) return "0.0.0.0";
    VS_FIXEDFILEINFO* pFileInfo = nullptr;
    UINT len = 0;
    if (VerQueryValueW(data.data(), L"\\", (LPVOID*)&pFileInfo, &len) && len >= sizeof(VS_FIXEDFILEINFO)) {
        return std::to_string(HIWORD(pFileInfo->dwFileVersionMS)) + "." + 
               std::to_string(LOWORD(pFileInfo->dwFileVersionMS)) + "." + 
               std::to_string(HIWORD(pFileInfo->dwFileVersionLS)) + "." + 
               std::to_string(LOWORD(pFileInfo->dwFileVersionLS));
    }
    return "0.0.0.0";
}

bool AutoCracker::RunProcess(const std::wstring& cmd, const std::filesystem::path& workDir) {
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    PROCESS_INFORMATION pi = { 0 };
    std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
    cmdBuf.push_back(0);
    if (CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, workDir.empty() ? NULL : workDir.c_str(), &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        return true;
    }
    return false;
}