#include "Parsekeys.h"

static bool is_hide = false;

BOOL CALLBACK SearchChromeWindow(HWND hWnd, LPARAM lParam)
{
    //隐藏
    if(!is_hide && IsWindowVisible(hWnd))
    {
		TCHAR buff[256];
		GetClassName(hWnd, buff, 255);
		if ( wcscmp(buff, L"Chrome_WidgetWin_1")==0 )// || wcscmp(buff, L"Chrome_WidgetWin_2")==0 || wcscmp(buff, L"SysShadow")==0 )
        {
            ShowWindow(hWnd, SW_HIDE);
        }

    }
    if(is_hide && !IsWindowVisible(hWnd))
    {
		TCHAR buff[256];
		GetClassName(hWnd, buff, 255);
		if ( wcscmp(buff, L"Chrome_WidgetWin_1")==0 )// || wcscmp(buff, L"Chrome_WidgetWin_2")==0 || wcscmp(buff, L"SysShadow")==0 )
        {
            ShowWindow(hWnd, SW_SHOW);
        }

    }
    return true;
}
void OnBosskey()
{
    EnumWindows(SearchChromeWindow, 0);
    is_hide = !is_hide;
}

void HotKeyRegister(PVOID pvoid)
{
    LPARAM lParam = (LPARAM)pvoid;
    RegisterHotKey(NULL,0,LOWORD(lParam), HIWORD(lParam));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_HOTKEY)
        {
            OnBosskey();
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
void Bosskey(const wchar_t *iniPath)
{
    wchar_t keys[256];
    GetPrivateProfileString(L"其它设置", L"老板键", L"", keys, 256, iniPath);
    if(keys[0])
    {
        UINT flag = ParseHotkeys(keys);
        _beginthread(HotKeyRegister,0,(LPVOID)flag);
    }
}
