wchar_t user_data_path[MAX_PATH];

typedef BOOL (WINAPI *pSHGetFolderPath)(
  _In_  HWND   hwndOwner,
  _In_  int    nFolder,
  _In_  HANDLE hToken,
  _In_  DWORD  dwFlags,
  _Out_ LPTSTR pszPath
);

pSHGetFolderPath RawSHGetFolderPath = NULL;

BOOL WINAPI MySHGetFolderPath(
  _In_  HWND   hwndOwner,
  _In_  int    nFolder,
  _In_  HANDLE hToken,
  _In_  DWORD  dwFlags,
  _Out_ LPTSTR pszPath
)
{
    BOOL result = RawSHGetFolderPath(hwndOwner, nFolder, hToken, dwFlags, pszPath);
    if (nFolder == CSIDL_LOCAL_APPDATA)
    {
        // 用户数据路径
        wcscpy(pszPath, user_data_path);
    }

    return result;
}

void CustomUserData(const wchar_t *iniPath)
{
    GetPrivateProfileStringW(L"基本设置", L"数据目录", L"", user_data_path, MAX_PATH, iniPath);

    // 扩展环境变量
    std::wstring path = ExpandEnvironmentPath(user_data_path);

    // exe路径
    wchar_t exeFolder[MAX_PATH];
    GetModuleFileNameW(NULL, exeFolder, MAX_PATH);
    PathRemoveFileSpecW(exeFolder);

    // 扩展%app%
    ReplaceStringInPlace(path, L"%app%", exeFolder);

    wcscpy(user_data_path, path.c_str());

    if(user_data_path[0])
    {
        //GetDefaultUserDataDirectory
        //*result = result->Append(chrome::kUserDataDirname);
        //const wchar_t kUserDataDirname[] = L"User Data";
        #ifdef _WIN64
        BYTE search[] = {0x48, 0x8B, 0xD1, 0xB9, 0x6E, 0x00, 0x00, 0x00, 0xE8};
        uint8_t *get_user_data = SearchModule(L"chrome.dll", search, sizeof(search));
        if (get_user_data && *(get_user_data + 17) == 0x0F && *(get_user_data + 18) == 0x84)
        {
            BYTE patch[] = { 0x90, 0xE9 };
            WriteMemory(get_user_data + 17, patch, sizeof(patch));
        }
        else if (get_user_data && *(get_user_data + 15) == 0x0F && *(get_user_data + 16) == 0x84)
        {
            BYTE patch[] = { 0x90, 0xE9 };
            WriteMemory(get_user_data + 15, patch, sizeof(patch));
        }
        else
        {
            DebugLog(L"patch user_data_path failed");
        }
        #else
        BYTE search[] = {0x57, 0x6A, 0x6E, 0xE8};
        uint8_t *get_user_data = SearchModule(L"chrome.dll", search, sizeof(search));
        if(get_user_data && *(get_user_data + 12) == 0x74)
        {
            BYTE patch[] = {0xEB};
            WriteMemory(get_user_data + 12, patch, sizeof(patch));
        }
        else
        {
            BYTE search[] = {0x6A, 0x6E, 0x8B, 0xD7 };
            uint8_t *get_user_data = SearchModule(L"chrome.dll", search, sizeof(search));
            if (get_user_data && *(get_user_data + 12) == 0x74)
            {
                BYTE patch[] = { 0xEB };
                WriteMemory(get_user_data + 12, patch, sizeof(patch));
            }
            else
            {
                DebugLog(L"patch user_data_path failed");
            }
        }
        #endif

        MH_STATUS status = MH_CreateHook(SHGetFolderPathW, MySHGetFolderPath, (LPVOID*)&RawSHGetFolderPath);
        if (status == MH_OK)
        {
            MH_EnableHook(SHGetFolderPathW);
        }
        else
        {
            DebugLog(L"MH_CreateHook CustomUserData failed:%d", status);
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
    if(GetPrivateProfileIntW(L"基本设置", L"便携化", 0, iniPath)==1)
    {
        HMODULE kernel32 = LoadLibraryW(L"kernel32.dll");
        if(kernel32)
        {
            PBYTE GetComputerNameW = (PBYTE)GetProcAddress(kernel32, "GetComputerNameW");
            PBYTE GetVolumeInformationW = (PBYTE)GetProcAddress(kernel32, "GetVolumeInformationW");

            MH_STATUS status = MH_CreateHook(GetComputerNameW, FakeGetComputerName, NULL);
            if (status == MH_OK)
            {
                MH_EnableHook(GetComputerNameW);
            }
            else
            {
                DebugLog(L"MH_CreateHook GetComputerNameW failed:%d", status);
            }
            status = MH_CreateHook(GetVolumeInformationW, FakeGetVolumeInformation, NULL);
            if (status == MH_OK)
            {
                MH_EnableHook(GetVolumeInformationW);
            }
            else
            {
                DebugLog(L"MH_CreateHook GetVolumeInformationW failed:%d", status);
            }
        }
    }
}



// bool PluginServiceImpl::NPAPIPluginsSupported()
// "Plugin.NPAPIStatus"
// chromium/content/browser/plugin_service_impl.cc
void RecoveryNPAPI(const wchar_t *iniPath)
{
    if(GetPrivateProfileIntW(L"基本设置", L"恢复NPAPI", 1, iniPath)==1)
    {
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
            else
            {
                DebugLog(L"patch npapi failed");
            }
        }
    }
}
