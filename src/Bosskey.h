#include "Parsekeys.h"

static bool is_hide = false;

HWND WndList[100];
int now = 0;

BOOL CALLBACK SearchChromeWindow(HWND hWnd, LPARAM lParam)
{
    //隐藏
    if(IsWindowVisible(hWnd))
    {
        TCHAR buff[256];
        GetClassName(hWnd, buff, 255);
        if ( wcscmp(buff, L"Chrome_WidgetWin_1")==0 )// || wcscmp(buff, L"Chrome_WidgetWin_2")==0 || wcscmp(buff, L"SysShadow")==0 )
        {
            if(now<100)
            {
                ShowWindow(hWnd, SW_HIDE);
                WndList[now] = hWnd;
                now++;
            }
        }

    }
    return true;
}
void OnBosskey()
{
    if(!is_hide)
    {
        EnumWindows(SearchChromeWindow, 0);
    }
    else
    {

        for(int i = now - 1;i>=0;i--)
        {
            HWND hWnd = WndList[i];
            ShowWindow(hWnd, SW_SHOW);
        }
        now = 0;
    }
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
bool Bosskey(const wchar_t *iniPath)
{
    wchar_t keys[256];
    GetPrivateProfileString(L"其它设置", L"老板键", L"", keys, 256, iniPath);
    if(keys[0])
    {
        UINT flag = ParseHotkeys(keys);
        _beginthread(HotKeyRegister,0,(LPVOID)flag);
    }
    return keys[0]!='\0';
}
