HRESULT FakeSHGetPropertyStoreForWindow(
  _In_  HWND   hwnd,
  _In_  REFIID riid,
  _Out_ void   **ppv
)
{
    return -1;
}

// 不让chrome使用SetAppIdForWindow
// chromium/ui/base/win/shell.cc
void RepairDoubleIcon(const wchar_t *iniPath)
{
    if(GetPrivateProfileInt(L"其它设置", L"修复任务栏双图标", 0, iniPath)==1)
    {
        HMODULE shell32 = LoadLibrary(L"shell32.dll");
        if(shell32)
        {
            PBYTE SHGetPropertyStoreForWindow = (PBYTE)GetProcAddress(shell32, "SHGetPropertyStoreForWindow");
            
            if (MH_CreateHook(SHGetPropertyStoreForWindow, FakeSHGetPropertyStoreForWindow, NULL) == MH_OK)
            {
                MH_EnableHook(SHGetPropertyStoreForWindow);
            }
        }
    }
}



BOOL WINAPI FakeVerifyVersionInfo(
  _In_ LPOSVERSIONINFOEX lpVersionInfo,
  _In_ DWORD             dwTypeMask,
  _In_ DWORDLONG         dwlConditionMask
)
{
    return 0;
}

// 让IsChromeMetroSupported强制返回false
// chromium/chrome/installer/util/shell_util.cc
void RepairDelegateExecute(const wchar_t *iniPath)
{
    if(GetPrivateProfileInt(L"其它设置", L"修复没有注册类", 0, iniPath)==1)
    {
        HMODULE kernel32 = LoadLibrary(L"kernel32.dll");
        if (kernel32)
        {
            PBYTE VerifyVersionInfoW = (PBYTE)GetProcAddress(kernel32, "VerifyVersionInfoW");
            
            if (MH_CreateHook(VerifyVersionInfoW, FakeVerifyVersionInfo, NULL) == MH_OK)
            {
                MH_EnableHook(VerifyVersionInfoW);
            }
        }
    }
}


BOOL WINAPI FakeGetComputerName(
  _Out_   LPTSTR  lpBuffer,
  _Inout_ LPDWORD lpnSize
)
{
    return 0;
}

BOOL WINAPI FakeGetVolumeInformation(
  _Out_   LPTSTR  lpBuffer,
  _Inout_ LPDWORD lpnSize
)
{
    return 0;
}

// 不让chrome使用GetComputerNameW，GetVolumeInformationW
// chromium/rlz/win/lib/machine_id_win.cc
void MakePortable(const wchar_t *iniPath)
{
    if(GetPrivateProfileInt(L"其它设置", L"便携化", 0, iniPath)==1)
    {
        HMODULE kernel32 = LoadLibrary(L"kernel32.dll");
        if(kernel32)
        {
            PBYTE GetComputerNameW = (PBYTE)GetProcAddress(kernel32, "GetComputerNameW");
            PBYTE GetVolumeInformationW = (PBYTE)GetProcAddress(kernel32, "GetVolumeInformationW");

            if (MH_CreateHook(GetComputerNameW, FakeGetComputerName, NULL) == MH_OK)
            {
                MH_EnableHook(GetComputerNameW);
            }
            if (MH_CreateHook(GetVolumeInformationW, FakeGetVolumeInformation, NULL) == MH_OK)
            {
                MH_EnableHook(GetVolumeInformationW);
            }
        }
    }
}
