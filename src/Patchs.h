HRESULT WINAPI FakeSHGetPropertyStoreForWindow(
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
    _In_opt_  LPCTSTR lpRootPathName,
    _Out_opt_ LPTSTR  lpVolumeNameBuffer,
    _In_      DWORD   nVolumeNameSize,
    _Out_opt_ LPDWORD lpVolumeSerialNumber,
    _Out_opt_ LPDWORD lpMaximumComponentLength,
    _Out_opt_ LPDWORD lpFileSystemFlags,
    _Out_opt_ LPTSTR  lpFileSystemNameBuffer,
    _In_      DWORD   nFileSystemNameSize
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



// bool PluginServiceImpl::NPAPIPluginsSupported()
// chromium/content/browser/plugin_service_impl.cc
void RecoveryNPAPI(const wchar_t *iniPath)
{
    if(GetPrivateProfileInt(L"其它设置", L"恢复NPAPI", 0, iniPath)==1)
    {
        #ifdef _WIN64
        BYTE search[] = {0x88, 0x47, 0x38, 0x84, 0xC0, 0x74, 0x05};
        #else
        BYTE search[] = {0x88, 0x46, 0x1C, 0x84, 0xC0, 0x74, 0x05};
        #endif

        uint8_t *npapi = SearchModule(L"chrome.dll", search, sizeof(search));
        if(npapi && *(npapi - 6) == 0xFF && *(npapi - 5) == 0x90)
        {
            #ifdef _WIN64
            BYTE patch[] = {0x31, 0xC0, 0xFF, 0xC0, 0x90, 0x90};
            #else
            BYTE patch[] = {0x31, 0xC0, 0x40, 0x90, 0x90, 0x90};
            #endif
            WriteMemory(npapi - 6, patch, sizeof(patch));
        }
    }
}



DWORD resources_pak_size = 0;

void PatchResourcesPak(uint8_t *buffer)
{
    BYTE search[] = "<div id=\"ntp-contents\">";
    BYTE patch[]  = "<div id=\"shuax--patch\">";

    uint8_t* pos = memmem(buffer, resources_pak_size, search, sizeof(search) - 1);
    if(pos)
    {
        memcpy(pos, patch, sizeof(patch) - 1);
    }
}

HANDLE resources_pak_map = NULL;

typedef HANDLE (WINAPI *pMapViewOfFile)(
  _In_ HANDLE hFileMappingObject,
  _In_ DWORD  dwDesiredAccess,
  _In_ DWORD  dwFileOffsetHigh,
  _In_ DWORD  dwFileOffsetLow,
  _In_ SIZE_T dwNumberOfBytesToMap
);

pMapViewOfFile RawMapViewOfFile = NULL;

HANDLE WINAPI MyMapViewOfFile(
  _In_ HANDLE hFileMappingObject,
  _In_ DWORD  dwDesiredAccess,
  _In_ DWORD  dwFileOffsetHigh,
  _In_ DWORD  dwFileOffsetLow,
  _In_ SIZE_T dwNumberOfBytesToMap
)
{
    if(hFileMappingObject == resources_pak_map)
    {
        // 修改属性为可修改
        LPVOID buffer = RawMapViewOfFile(hFileMappingObject, FILE_MAP_COPY, dwFileOffsetHigh,
            dwFileOffsetLow, dwNumberOfBytesToMap);

        // 不再需要hook
        resources_pak_map = NULL;
        MH_DisableHook(MapViewOfFile);

        if(buffer)
        {
            // 给resources.pak打补丁
            PatchResourcesPak((BYTE*)buffer);
        }

        return buffer;
    }

    return RawMapViewOfFile(hFileMappingObject, dwDesiredAccess, dwFileOffsetHigh,
        dwFileOffsetLow, dwNumberOfBytesToMap);
}

HANDLE resources_pak_file = NULL;

typedef HANDLE (WINAPI *pCreateFileMapping)(
  _In_     HANDLE                hFile,
  _In_opt_ LPSECURITY_ATTRIBUTES lpAttributes,
  _In_     DWORD                 flProtect,
  _In_     DWORD                 dwMaximumSizeHigh,
  _In_     DWORD                 dwMaximumSizeLow,
  _In_opt_ LPCTSTR               lpName
);

pCreateFileMapping RawCreateFileMapping = NULL;

HANDLE WINAPI MyCreateFileMapping(
  _In_     HANDLE                hFile,
  _In_opt_ LPSECURITY_ATTRIBUTES lpAttributes,
  _In_     DWORD                 flProtect,
  _In_     DWORD                 dwMaximumSizeHigh,
  _In_     DWORD                 dwMaximumSizeLow,
  _In_opt_ LPCTSTR               lpName
)
{
    if(hFile == resources_pak_file)
    {
        // 修改属性为可修改
        resources_pak_map = RawCreateFileMapping(hFile, lpAttributes, PAGE_WRITECOPY,
            dwMaximumSizeHigh, dwMaximumSizeLow, lpName);

        // 不再需要hook
        resources_pak_file = NULL;
        MH_DisableHook(CreateFileMappingW);

        if (MH_CreateHook(MapViewOfFile, MyMapViewOfFile, (LPVOID*)&RawMapViewOfFile) == MH_OK)
        {
            MH_EnableHook(MapViewOfFile);
        }

        return resources_pak_map;
    }
    return RawCreateFileMapping(hFile, lpAttributes, flProtect, dwMaximumSizeHigh,
        dwMaximumSizeLow, lpName);
}

typedef HANDLE (WINAPI *pCreateFile)(
  _In_     LPCTSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile
);

pCreateFile RawCreateFile = NULL;

HANDLE WINAPI MyCreateFile(
  _In_     LPCTSTR               lpFileName,
  _In_     DWORD                 dwDesiredAccess,
  _In_     DWORD                 dwShareMode,
  _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  _In_     DWORD                 dwCreationDisposition,
  _In_     DWORD                 dwFlagsAndAttributes,
  _In_opt_ HANDLE                hTemplateFile
)
{
    HANDLE file = RawCreateFile(lpFileName, dwDesiredAccess, dwShareMode,
        lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes,
        hTemplateFile);

    if(isEndWith(lpFileName, L"resources.pak"))
    {
        resources_pak_file = file;
        resources_pak_size = GetFileSize(resources_pak_file, NULL);

        if (MH_CreateHook(CreateFileMappingW, MyCreateFileMapping, (LPVOID*)&RawCreateFileMapping) == MH_OK)
        {
            MH_EnableHook(CreateFileMappingW);
        }

        // 不再需要hook
        // MH_DisableHook(CreateFileW);
    }
    return file;
}

void BlankNewTab(const wchar_t *iniPath)
{
    if(GetPrivateProfileInt(L"其它设置", L"新标签空白", 0, iniPath)==1)
    {
        if (MH_CreateHook(CreateFileW, MyCreateFile, (LPVOID*)&RawCreateFile) == MH_OK)
        {
            MH_EnableHook(CreateFileW);
        }
    }
}