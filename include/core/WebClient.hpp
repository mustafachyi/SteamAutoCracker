#ifndef WEB_CLIENT_HPP
#define WEB_CLIENT_HPP

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

class WebClient {
public:
    static std::string Fetch(const std::wstring& host, const std::wstring& path) {
        HINTERNET hSession = GetSession();
        if (!hSession) return {};

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), INTERNET_DEFAULT_HTTPS_PORT, 0);
        if (!hConnect) return {};

        std::string response;
        response.reserve(4096);
        
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        
        if (hRequest) {
            DWORD redirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
            WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &redirectPolicy, sizeof(redirectPolicy));

            if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(hRequest, nullptr)) {
                
                DWORD size = 0, downloaded = 0;
                std::vector<char> buffer;
                
                while (WinHttpQueryDataAvailable(hRequest, &size) && size > 0) {
                    if (buffer.size() < size) buffer.resize(size);
                    if (WinHttpReadData(hRequest, buffer.data(), size, &downloaded)) {
                        response.append(buffer.data(), downloaded);
                    }
                }
            }
            WinHttpCloseHandle(hRequest);
        }
        WinHttpCloseHandle(hConnect);
        return response;
    }

private:
    static HINTERNET GetSession() {
        static HINTERNET hSession = WinHttpOpen(L"SAC_Core/3.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        return hSession;
    }
};

#endif