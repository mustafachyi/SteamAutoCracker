#include "../include/ui/MainWindow.hpp"
#include <commctrl.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_WIN95_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    MainWindow win;
    if (!win.Create(hInstance, nCmdShow)) return 0;
    win.RunMessageLoop();
    return 0;
}