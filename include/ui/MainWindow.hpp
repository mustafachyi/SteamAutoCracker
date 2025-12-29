#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <windows.h>
#include <string>
#include <vector>
#include <filesystem>
#include <shobjidl.h>
#include <deque>
#include "../core/AppIdResolver.hpp"
#include "../core/AutoCracker.hpp"

enum class AppState {
    SelectFolder,
    AlreadyCracked,
    SearchApp,
    Fetching,
    Ready,
    Cracking,
    Done,
    Error
};

namespace SAC_UI {
    constexpr int IDC_STATUS = 100;
    constexpr int IDC_BTN_BROWSE = 101;
    constexpr int IDC_BTN_SEARCH = 102;
    constexpr int IDC_LIST_RESULTS = 103;
    constexpr int IDC_BTN_CRACK = 104;
    constexpr int IDC_EDIT_SEARCH = 105;
    constexpr int IDC_BTN_RESTORE = 106;
    constexpr int IDC_LIST_CRACKS = 107;
    constexpr int IDC_LBL_METHOD = 108;
    constexpr int IDC_LIST_LOGS = 109;
}

class MainWindow {
public:
    MainWindow();
    ~MainWindow();
    bool Create(HINSTANCE hInstance, int nCmdShow);
    void RunMessageLoop();

    void ApplyDarkTheme(HWND target);
    void OnDrawItem(LPARAM lParam);
    void SetState(AppState newState);
    void UpdateLayout();
    
    void OnBrowseFolder();
    void OnSearch();
    void OnCrack();
    void OnRestore();
    void OnFinish();
    void FetchGameDetails(std::uint32_t appId);
    void OnGameInfoReady(GameInfo* info);
    void OnSearchResultReady(std::vector<AppSearchResult>* results);
    void OnStatusUpdate(const std::wstring* status);
    void OnCrackFinished(bool success);
    void OnTimer();
    void Log(const std::wstring& msg, bool showTime = true);

    static std::wstring Utf8ToWide(const std::string& str);

    HWND m_hwnd;
    HWND m_btnBrowse;
    HWND m_lblStatus;
    HWND m_lblMethod;
    HWND m_editSearch;
    HWND m_btnSearch;
    HWND m_listResults;
    HWND m_btnCrack;
    HWND m_btnRestore;
    HWND m_listCracks;
    HWND m_listLogs;
    
    HBRUSH m_hBrushBg;
    HBRUSH m_hBrushBtnBase;
    HBRUSH m_hBrushBtnHover;
    HBRUSH m_hBrushBtnSel;
    HBRUSH m_hBrushBtnDis;
    HBRUSH m_hBrushListSel;
    HBRUSH m_hBrushBorder;
    HBRUSH m_hBrushLogBg;

    std::filesystem::path m_selectedPath;
    std::uint32_t m_appId;
    AppState m_state;
    AppIdResolver m_resolver;
    GameInfo m_gameInfo;
    std::vector<std::string> m_crackFolders;
    std::deque<std::pair<std::wstring, bool>> m_logQueue;

    static const UINT WM_SAC_GAME_INFO = WM_USER + 1;
    static const UINT WM_SAC_SEARCH_RESULT = WM_USER + 2;
    static const UINT WM_SAC_UPDATE_STATUS = WM_USER + 3;
    static const UINT WM_SAC_CRACK_DONE = WM_USER + 4;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    static LRESULT CALLBACK LogSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
    void InitDarkMode();
    void PopulateCrackList();
};

#endif