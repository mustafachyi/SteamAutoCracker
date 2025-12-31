#include "../../include/ui/MainWindow.hpp"
#include "../../include/core/Config.hpp"
#include <dwmapi.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <windowsx.h>
#include <thread>
#include <algorithm>
#include <cstdio>
#include <shlobj.h>

namespace {
    using fnSetPreferredAppMode = void(WINAPI*)(int);
    using fnAllowDarkModeForWindow = bool(WINAPI*)(HWND, bool);
}

MainWindow::MainWindow() : m_hwnd(nullptr), m_btnBrowse(nullptr), m_lblStatus(nullptr), m_lblMethod(nullptr),
                           m_editSearch(nullptr), m_btnSearch(nullptr), m_listResults(nullptr), 
                           m_btnCrack(nullptr), m_btnRestore(nullptr), m_listCracks(nullptr), m_listLogs(nullptr),
                           m_appId(0), m_state(AppState::SelectFolder) {
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    InitDarkMode();
    m_hBrushBg = CreateSolidBrush(RGB(25, 25, 25));
    m_hBrushBtnBase = CreateSolidBrush(RGB(55, 55, 55));
    m_hBrushBtnHover = CreateSolidBrush(RGB(75, 75, 75));
    m_hBrushBtnSel = CreateSolidBrush(RGB(85, 85, 85));
    m_hBrushBtnDis = CreateSolidBrush(RGB(40, 40, 40));
    m_hBrushListSel = CreateSolidBrush(RGB(85, 85, 85));
    m_hBrushBorder = CreateSolidBrush(RGB(80, 80, 80));
    m_hBrushLogBg = CreateSolidBrush(RGB(20, 20, 20));
}

MainWindow::~MainWindow() {
    DeleteObject(m_hBrushBg); DeleteObject(m_hBrushBtnBase); DeleteObject(m_hBrushBtnHover);
    DeleteObject(m_hBrushBtnSel); DeleteObject(m_hBrushBtnDis); DeleteObject(m_hBrushListSel);
    DeleteObject(m_hBrushBorder); DeleteObject(m_hBrushLogBg);
    CoUninitialize();
}

bool MainWindow::Create(HINSTANCE hInstance, int nCmdShow) {
    std::string err;
    if (!AutoCracker::ValidateTools(err)) {
        MessageBoxW(nullptr, std::wstring(err.begin(), err.end()).c_str(), L"Error", MB_ICONERROR);
        return false;
    }

    const wchar_t CLASS_NAME[] = L"SAC_MainWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = MainWindow::WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = m_hBrushBg;
    RegisterClassW(&wc);

    m_hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, CLASS_NAME, L"", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, 
                             CW_USEDEFAULT, CW_USEDEFAULT, 600, 450, nullptr, nullptr, hInstance, this);
    if (!m_hwnd) return false;

    ApplyDarkTheme(m_hwnd);

    auto CreateCtrl = [&](const wchar_t* cls, const wchar_t* txt, DWORD s, int x, int y, int w, int h, int id) {
        return CreateWindowExW(0, cls, txt, WS_CHILD | s, x, y, w, h, m_hwnd, (HMENU)(UINT_PTR)id, hInstance, nullptr);
    };

    m_lblStatus = CreateCtrl(L"STATIC", L"", WS_VISIBLE | SS_OWNERDRAW, 0, 0, 0, 0, SAC_UI::IDC_STATUS);
    m_lblMethod = CreateCtrl(L"STATIC", L"Cracking Method:", SS_OWNERDRAW, 0, 0, 0, 0, SAC_UI::IDC_LBL_METHOD);
    m_btnBrowse = CreateCtrl(L"BUTTON", L"Change", BS_PUSHBUTTON | BS_OWNERDRAW, 0, 0, 0, 0, SAC_UI::IDC_BTN_BROWSE);
    m_btnCrack = CreateCtrl(L"BUTTON", L"Crack", BS_PUSHBUTTON | BS_OWNERDRAW, 0, 0, 0, 0, SAC_UI::IDC_BTN_CRACK);
    m_btnRestore = CreateCtrl(L"BUTTON", L"Restore", BS_PUSHBUTTON | BS_OWNERDRAW, 0, 0, 0, 0, SAC_UI::IDC_BTN_RESTORE);
    m_listCracks = CreateCtrl(L"LISTBOX", L"", WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS, 0, 0, 0, 0, SAC_UI::IDC_LIST_CRACKS);
    m_listLogs = CreateCtrl(L"LISTBOX", L"", WS_BORDER | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS, 0, 0, 0, 0, SAC_UI::IDC_LIST_LOGS);
    m_editSearch = CreateCtrl(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL, 22, 70, 440, 32, SAC_UI::IDC_EDIT_SEARCH);
    m_btnSearch = CreateCtrl(L"BUTTON", L"Search", BS_PUSHBUTTON | BS_OWNERDRAW | WS_DISABLED, 472, 70, 90, 32, SAC_UI::IDC_BTN_SEARCH);
    m_listResults = CreateCtrl(L"LISTBOX", L"", WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_OWNERDRAWFIXED | LBS_HASSTRINGS, 22, 115, 540, 275, SAC_UI::IDC_LIST_RESULTS);

    SetWindowSubclass(m_editSearch, EditSubclassProc, 0, 0);
    SetWindowSubclass(m_listLogs, LogSubclassProc, 0, (DWORD_PTR)this);
    
    SendMessageW(m_listResults, LB_SETITEMHEIGHT, 0, 24);
    SendMessageW(m_listCracks, LB_SETITEMHEIGHT, 0, 24);
    SendMessageW(m_listLogs, LB_SETITEMHEIGHT, 0, 20);

    HFONT hFont = CreateFontW(19, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontSmall = CreateFontW(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT hFontMono = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
    
    const HWND ctrls[] = { m_lblStatus, m_btnBrowse, m_btnCrack, m_btnRestore, m_listCracks, m_editSearch, m_btnSearch, m_listResults };
    for (HWND h : ctrls) SendMessageW(h, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(m_lblMethod, WM_SETFONT, (WPARAM)hFontSmall, TRUE);
    SendMessageW(m_listLogs, WM_SETFONT, (WPARAM)(hFontMono ? hFontMono : hFontSmall), TRUE);

    ApplyDarkTheme(m_editSearch); ApplyDarkTheme(m_listResults); ApplyDarkTheme(m_listCracks); ApplyDarkTheme(m_listLogs);
    PopulateCrackList();

    SetState(AppState::SelectFolder);

    ShowWindow(m_hwnd, nCmdShow);
    return true;
}

void MainWindow::RunMessageLoop() {
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
}

void MainWindow::InitDarkMode() {
    if (HMODULE h = GetModuleHandleW(L"uxtheme.dll")) {
        if (auto f = (fnSetPreferredAppMode)GetProcAddress(h, MAKEINTRESOURCEA(135))) f(2);
    }
}

void MainWindow::ApplyDarkTheme(HWND target) {
    BOOL v = TRUE;
    DwmSetWindowAttribute(target, 20, &v, sizeof(v));
    SetWindowTheme(target, L"DarkMode_CFD", nullptr);
    if (HMODULE h = GetModuleHandleW(L"uxtheme.dll")) {
        if (auto f = (fnAllowDarkModeForWindow)GetProcAddress(h, MAKEINTRESOURCEA(133))) f(target, true);
    }
}

void MainWindow::PopulateCrackList() {
    SendMessageW(m_listCracks, LB_RESETCONTENT, 0, 0);
    m_crackFolders.clear();
    auto emuDir = std::filesystem::current_path() / "tools" / "sac_emu";
    if (!std::filesystem::exists(emuDir)) return;

    const std::pair<std::wstring, std::string> entries[] = { { L"Ali213", "game_ali213" }, { L"Goldberg", "game_goldberg" }, { L"CreamAPI", "dlc_creamapi" } };
    for (const auto& entry : entries) {
        if (std::filesystem::exists(emuDir / entry.second)) {
            SendMessageW(m_listCracks, LB_ADDSTRING, 0, (LPARAM)entry.first.c_str());
            m_crackFolders.push_back(entry.second);
        }
    }
    SendMessageW(m_listCracks, LB_SETCURSEL, 0, 0);
}

std::wstring MainWindow::Utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring wstr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &wstr[0], size);
    if (size > 0) wstr.resize(size - 1);
    return wstr;
}

void MainWindow::Log(const std::wstring& msg, bool showTime) {
    if (m_listLogs) {
        bool start = m_logQueue.empty();
        m_logQueue.push_back({msg, showTime});
        if (start) SetTimer(m_hwnd, 1, 20, nullptr);
    }
}

void MainWindow::OnTimer() {
    if (m_logQueue.empty()) {
        KillTimer(m_hwnd, 1);
        return;
    }

    const auto& item = m_logQueue.front();
    if (item.second) {
        SYSTEMTIME st; GetLocalTime(&st);
        wchar_t timeBuf[16];
        swprintf_s(timeBuf, 16, L"[%02d:%02d:%02d] ", st.wHour, st.wMinute, st.wSecond);
        std::wstring fullMsg = timeBuf + item.first;
        SendMessageW(m_listLogs, LB_ADDSTRING, 0, (LPARAM)fullMsg.c_str());
    } else {
        SendMessageW(m_listLogs, LB_ADDSTRING, 0, (LPARAM)item.first.c_str());
    }
    
    SendMessageW(m_listLogs, LB_SETTOPINDEX, SendMessageW(m_listLogs, LB_GETCOUNT, 0, 0) - 1, 0);
    m_logQueue.pop_front();
    
    if (m_logQueue.empty()) KillTimer(m_hwnd, 1);
}

void MainWindow::OnDrawItem(LPARAM lParam) {
    LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, RGB(255, 255, 255));
    if (dis->CtlType == ODT_BUTTON) {
        HBRUSH bg = (dis->itemState & ODS_DISABLED) ? m_hBrushBtnDis : 
                    ((dis->itemState & ODS_SELECTED) ? m_hBrushBtnSel : 
                    ((dis->itemState & ODS_HOTLIGHT) ? m_hBrushBtnHover : m_hBrushBtnBase));
        FillRect(dis->hDC, &dis->rcItem, bg);
        FrameRect(dis->hDC, &dis->rcItem, m_hBrushBorder);
        if (dis->itemState & ODS_DISABLED) SetTextColor(dis->hDC, RGB(128, 128, 128));
        wchar_t t[128]; GetWindowTextW(dis->hwndItem, t, 128);
        DrawTextW(dis->hDC, t, -1, &dis->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    } else if (dis->CtlType == ODT_LISTBOX) {
        bool isLog = (dis->hwndItem == m_listLogs);
        if ((int)dis->itemID == -1) { FillRect(dis->hDC, &dis->rcItem, isLog ? m_hBrushLogBg : m_hBrushBg); return; }
        
        FillRect(dis->hDC, &dis->rcItem, isLog ? m_hBrushLogBg : ((dis->itemState & ODS_SELECTED) ? m_hBrushListSel : m_hBrushBg));
        
        wchar_t t[512]; SendMessageW(dis->hwndItem, LB_GETTEXT, dis->itemID, (LPARAM)t);
        RECT rc = dis->rcItem; rc.left += 6;
        if (isLog) SetTextColor(dis->hDC, RGB(200, 200, 200));
        DrawTextW(dis->hDC, t, -1, &rc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else if (dis->CtlType == ODT_STATIC) {
        FillRect(dis->hDC, &dis->rcItem, m_hBrushBg);
        wchar_t t[1024]; GetWindowTextW(dis->hwndItem, t, 1024);
        UINT fmt = DT_SINGLELINE | DT_VCENTER;
        if (m_state == AppState::SelectFolder || m_state == AppState::AlreadyCracked) fmt |= DT_CENTER;
        else fmt |= DT_LEFT;
        if (m_state == AppState::SearchApp) fmt |= DT_END_ELLIPSIS;
        DrawTextW(dis->hDC, t, -1, &dis->rcItem, fmt);
    }
}

void MainWindow::SetState(AppState newState) {
    m_state = newState;
    std::wstring statusText;

    switch (m_state) {
        case AppState::SelectFolder: statusText = L"Select Game Directory"; break;
        case AppState::AlreadyCracked: statusText = L"Game detected as already cracked"; break;
        case AppState::SearchApp: statusText = m_selectedPath.filename().wstring(); break;
        default: break;
    }

    SetWindowTextW(m_lblStatus, statusText.c_str());
    UpdateLayout();
}

void MainWindow::UpdateLayout() {
    RECT rc; GetClientRect(m_hwnd, &rc);
    const HWND ctrls[] = { m_editSearch, m_btnSearch, m_listResults, m_btnCrack, m_btnRestore, m_btnBrowse, m_listCracks, m_lblMethod, m_listLogs, m_lblStatus };
    for (HWND h : ctrls) ShowWindow(h, SW_HIDE);

    int centerX = rc.right / 2;
    int midY = (rc.bottom - 100) / 2;

    if (m_state == AppState::Cracking || m_state == AppState::Fetching || m_state == AppState::Done || m_state == AppState::Error) {
        SetWindowPos(m_listLogs, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
        ShowWindow(m_listLogs, SW_SHOW);
        if (m_state == AppState::Done || m_state == AppState::Error) SetFocus(m_listLogs);
        InvalidateRect(m_hwnd, NULL, TRUE);
        return;
    }

    if (m_state == AppState::SelectFolder) {
        SetWindowPos(m_lblStatus, NULL, 30, midY - 32, rc.right - 60, 40, SWP_NOZORDER);
        SetWindowPos(m_btnBrowse, NULL, centerX - 100, midY + 28, 200, 40, SWP_NOZORDER);
        SetWindowTextW(m_btnBrowse, L"Game Folder");
        ShowWindow(m_lblStatus, SW_SHOW); ShowWindow(m_btnBrowse, SW_SHOW);
        return;
    } 
    
    if (m_state == AppState::SearchApp) {
        SetWindowPos(m_lblStatus, NULL, 22, 15, 440, 32, SWP_NOZORDER);
        SetWindowPos(m_btnBrowse, NULL, 472, 15, 90, 32, SWP_NOZORDER);
        SetWindowTextW(m_btnBrowse, L"Change");
        ShowWindow(m_lblStatus, SW_SHOW); ShowWindow(m_btnBrowse, SW_SHOW); 
        ShowWindow(m_editSearch, SW_SHOW); ShowWindow(m_btnSearch, SW_SHOW); ShowWindow(m_listResults, SW_SHOW);
        return;
    }

    if (m_state == AppState::AlreadyCracked) {
        int totalW = 220; 
        int startX = centerX - (totalW / 2);
        SetWindowPos(m_lblStatus, NULL, 0, midY - 35, rc.right, 30, SWP_NOZORDER);
        SetWindowPos(m_btnRestore, NULL, startX, midY, 100, 35, SWP_NOZORDER);
        SetWindowPos(m_btnBrowse, NULL, startX + 110, midY, 100, 35, SWP_NOZORDER);
        SetWindowTextW(m_btnBrowse, L"Change");
        ShowWindow(m_lblStatus, SW_SHOW); ShowWindow(m_btnRestore, SW_SHOW); ShowWindow(m_btnBrowse, SW_SHOW);
    } else if (m_state == AppState::Ready) {
        int listW = 200, listH = 80, btnW = 100;
        int startX = centerX - (listW + 20 + btnW) / 2;
        int startY = midY - 40;
        SetWindowPos(m_lblMethod, NULL, startX, startY - 20, listW, 20, SWP_NOZORDER);
        SetWindowPos(m_listCracks, NULL, startX, startY, listW, listH, SWP_NOZORDER);
        SetWindowPos(m_btnCrack, NULL, startX + listW + 20, startY, btnW, 35, SWP_NOZORDER);
        SetWindowPos(m_btnBrowse, NULL, startX + listW + 20, startY + 45, btnW, 35, SWP_NOZORDER);
        SetWindowTextW(m_btnBrowse, L"Change");
        ShowWindow(m_lblMethod, SW_SHOW); ShowWindow(m_listCracks, SW_SHOW);
        ShowWindow(m_btnCrack, SW_SHOW); ShowWindow(m_btnBrowse, SW_SHOW);
    }
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void MainWindow::OnBrowseFolder() {
    IFileOpenDialog* pFileOpen;
    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen)))) {
        DWORD dwOptions;
        pFileOpen->GetOptions(&dwOptions);
        pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);

        if (!Config::Instance().lastFolder.empty()) {
            IShellItem* pDefaultFolder = nullptr;
            if (SUCCEEDED(SHCreateItemFromParsingName(Config::Instance().lastFolder.c_str(), NULL, IID_PPV_ARGS(&pDefaultFolder)))) {
                pFileOpen->SetFolder(pDefaultFolder);
                pDefaultFolder->Release();
            }
        }

        if (SUCCEEDED(pFileOpen->Show(m_hwnd))) {
            IShellItem* pItem;
            if (SUCCEEDED(pFileOpen->GetResult(&pItem))) {
                PWSTR pszFilePath;
                if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath))) {
                    m_selectedPath = pszFilePath;
                    
                    std::filesystem::path p(pszFilePath);
                    Config::Instance().lastFolder = p.parent_path().wstring();
                    Config::Instance().Save();

                    SendMessageW(m_listResults, LB_RESETCONTENT, 0, 0);
                    SendMessageW(m_listLogs, LB_RESETCONTENT, 0, 0);
                    SetWindowTextW(m_editSearch, L"");
                    if (AutoCracker::IsCracked(m_selectedPath)) SetState(AppState::AlreadyCracked);
                    else {
                        uint32_t id = m_resolver.ResolveFromFolder(m_selectedPath);
                        if (id != 0) FetchGameDetails(id);
                        else SetState(AppState::SearchApp);
                    }
                    CoTaskMemFree(pszFilePath);
                }
                pItem->Release();
            }
        }
        pFileOpen->Release();
    }
}

void MainWindow::OnSearch() {
    wchar_t buf[256];
    GetWindowTextW(m_editSearch, buf, 256);
    std::wstring ws(buf);
    ws.erase(0, ws.find_first_not_of(L" \t\r\n"));
    ws.erase(ws.find_last_not_of(L" \t\r\n") + 1);
    if (ws.empty()) return;
    if (std::all_of(ws.begin(), ws.end(), ::iswdigit)) {
        FetchGameDetails(std::stoul(ws));
        return;
    }
    EnableWindow(m_btnSearch, FALSE);
    SendMessageW(m_listResults, LB_RESETCONTENT, 0, 0);
    int size = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string input(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &input[0], size, nullptr, nullptr);
    std::thread([this, input]() {
        auto results = new std::vector<AppSearchResult>(m_resolver.SearchByName(input));
        PostMessageW(m_hwnd, WM_SAC_SEARCH_RESULT, 0, (LPARAM)results);
    }).detach();
}

void MainWindow::OnCrack() {
    int idx = (int)SendMessageW(m_listCracks, LB_GETCURSEL, 0, 0);
    std::string sCrack = (idx >= 0 && idx < (int)m_crackFolders.size()) ? m_crackFolders[idx] : "game_ali213";
    SetState(AppState::Cracking);
    SendMessageW(m_listLogs, LB_RESETCONTENT, 0, 0);
    Log(L"Target: " + (m_gameInfo.name.empty() ? m_selectedPath.filename().wstring() : Utf8ToWide(m_gameInfo.name)));
    std::thread([this, sCrack]() {
        AutoCracker::Settings settings{ m_selectedPath, sCrack, sCrack.find("game_") == 0 };
        auto cb = [this](const std::string& s) {
            PostMessageW(m_hwnd, WM_SAC_UPDATE_STATUS, 0, (LPARAM)new std::wstring(Utf8ToWide(s)));
        };
        PostMessageW(m_hwnd, WM_SAC_CRACK_DONE, (WPARAM)AutoCracker::Apply(m_gameInfo, settings, cb), 0);
    }).detach();
}

void MainWindow::OnRestore() {
    int idx = (int)SendMessageW(m_listCracks, LB_GETCURSEL, 0, 0);
    std::string sCrack = (idx >= 0 && idx < (int)m_crackFolders.size()) ? m_crackFolders[idx] : "game_ali213";
    SetState(AppState::Cracking);
    SendMessageW(m_listLogs, LB_RESETCONTENT, 0, 0);
    Log(L"Target: " + (m_gameInfo.name.empty() ? m_selectedPath.filename().wstring() : Utf8ToWide(m_gameInfo.name)));
    std::thread([this, sCrack]() {
        AutoCracker::Settings settings{ m_selectedPath, sCrack, false };
        auto cb = [this](const std::string& s) {
            PostMessageW(m_hwnd, WM_SAC_UPDATE_STATUS, 0, (LPARAM)new std::wstring(Utf8ToWide(s)));
        };
        PostMessageW(m_hwnd, WM_SAC_CRACK_DONE, (WPARAM)AutoCracker::Revert(settings, cb), 0);
    }).detach();
}

void MainWindow::OnFinish() {
    KillTimer(m_hwnd, 1);
    m_logQueue.clear();
    m_gameInfo = {};
    m_appId = 0;
    m_selectedPath.clear();
    SendMessageW(m_listLogs, LB_RESETCONTENT, 0, 0);
    SetState(AppState::SelectFolder);
}

void MainWindow::FetchGameDetails(std::uint32_t appId) {
    std::thread([this, appId]() {
        auto cb = [this](const std::string& s) {
            PostMessageW(m_hwnd, WM_SAC_UPDATE_STATUS, 0, (LPARAM)new std::wstring(Utf8ToWide(s)));
        };
        PostMessageW(m_hwnd, WM_SAC_GAME_INFO, 0, (LPARAM)new GameInfo(m_resolver.GetGameInfo(appId, cb)));
    }).detach();
}

void MainWindow::OnStatusUpdate(const std::wstring* s) { 
    if (s) { 
        Log(*s); 
        delete s; 
    } 
}

void MainWindow::OnGameInfoReady(GameInfo* info) { 
    if (info) { 
        m_gameInfo = *info; 
        m_appId = m_gameInfo.id; 
        delete info; 
    } 
    if (m_gameInfo.id == 0) {
        SetState(AppState::Error);
        Log(L"Failed to fetch game metadata.");
        Log(L"Press Enter to return.", false);
    } else {
        SetState(AppState::Ready);
    }
}

void MainWindow::OnSearchResultReady(std::vector<AppSearchResult>* results) { 
    if (results) { 
        for (const auto& res : *results) 
            SendMessageW(m_listResults, LB_ADDSTRING, 0, (LPARAM)(Utf8ToWide(res.name) + L" [" + std::to_wstring(res.id) + L"]").c_str()); 
        delete results; 
    } 
    EnableWindow(m_btnSearch, TRUE); 
}

void MainWindow::OnCrackFinished(bool s) { 
    SetState(s ? AppState::Done : AppState::Error); 
    Log(L"Operation finished.", false);
    Log(L"Press Enter to return.", false);
}

LRESULT CALLBACK MainWindow::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    MainWindow* p = (uMsg == WM_NCCREATE) ? (MainWindow*)((CREATESTRUCTW*)lParam)->lpCreateParams : (MainWindow*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (uMsg == WM_NCCREATE) SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)p);
    if (!p) return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    
    switch (uMsg) {
        case WM_TIMER: p->OnTimer(); return 0;
        case WM_SAC_GAME_INFO: p->OnGameInfoReady((GameInfo*)lParam); return 0;
        case WM_SAC_SEARCH_RESULT: p->OnSearchResultReady((std::vector<AppSearchResult>*)lParam); return 0;
        case WM_SAC_UPDATE_STATUS: p->OnStatusUpdate((std::wstring*)lParam); return 0;
        case WM_SAC_CRACK_DONE: p->OnCrackFinished((bool)wParam); return 0;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc; GetClientRect(hwnd, &rc);
            rc.bottom = 1;
            HBRUSH hBr = (HBRUSH)GetStockObject(DC_BRUSH);
            SetDCBrushColor(hdc, RGB(23, 22, 21));
            FillRect(hdc, &rc, hBr);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DRAWITEM: p->OnDrawItem(lParam); return TRUE;
        case WM_CTLCOLORSTATIC: case WM_CTLCOLORLISTBOX: case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            if ((HWND)lParam == p->m_listLogs) {
                SetTextColor(hdc, RGB(200, 200, 200));
                SetBkColor(hdc, RGB(20, 20, 20));
                return (LRESULT)p->m_hBrushLogBg;
            }
            SetTextColor(hdc, RGB(255, 255, 255)); 
            SetBkColor(hdc, RGB(25, 25, 25)); 
            return (LRESULT)p->m_hBrushBg;
        }
        case WM_COMMAND:
            switch(LOWORD(wParam)) {
                case SAC_UI::IDC_BTN_BROWSE: p->OnBrowseFolder(); break;
                case SAC_UI::IDC_BTN_SEARCH: p->OnSearch(); break;
                case SAC_UI::IDC_BTN_CRACK: p->OnCrack(); break;
                case SAC_UI::IDC_BTN_RESTORE: p->OnRestore(); break;
                case SAC_UI::IDC_EDIT_SEARCH: if(HIWORD(wParam) == EN_CHANGE) EnableWindow(p->m_btnSearch, GetWindowTextLengthW(p->m_editSearch) > 0); break;
                case SAC_UI::IDC_LIST_RESULTS: if(HIWORD(wParam) == LBN_DBLCLK) {
                    int sel = (int)SendMessageW(p->m_listResults, LB_GETCURSEL, 0, 0);
                    if (sel != LB_ERR) {
                        wchar_t buf[256]; SendMessageW(p->m_listResults, LB_GETTEXT, sel, (LPARAM)buf);
                        std::wstring s(buf); size_t start = s.find_last_of(L"[") + 1;
                        if (start != std::wstring::npos) p->FetchGameDetails(std::stoul(s.substr(start, s.find_last_of(L"]") - start)));
                    }
                } break;
            } return 0;
        case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::EditSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR) {
    if (uMsg == WM_KEYDOWN && wParam == VK_RETURN) {
        if (IsWindowEnabled(GetDlgItem(GetParent(hwnd), SAC_UI::IDC_BTN_SEARCH))) 
            SendMessageW(GetParent(hwnd), WM_COMMAND, SAC_UI::IDC_BTN_SEARCH, 0);
        return 0;
    }
    if (uMsg == WM_CHAR) {
        if (wParam == VK_RETURN) return 0;
        if (wParam == 1) {
            SendMessageW(hwnd, EM_SETSEL, 0, -1);
            return 0;
        }
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MainWindow::LogSubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData) {
    MainWindow* p = (MainWindow*)dwRefData;
    if (uMsg == WM_KEYDOWN && wParam == VK_RETURN) {
        if (p && (p->m_state == AppState::Done || p->m_state == AppState::Error)) {
            p->OnFinish();
            return 0;
        }
    }
    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}