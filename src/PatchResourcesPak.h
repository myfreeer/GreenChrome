bool MakeNewTabBlank = false;

DWORD resources_pak_size = 0;

void BlankNewTab(uint8_t *buffer)
{
    BYTE search[] = R"(<div id="ntp-contents">)";
    BYTE patch[]  = R"(<div id="shuax--patch">)";

    uint8_t* pos = memmem(buffer, resources_pak_size, search, sizeof(search) - 1);
    if(pos)
    {
        memcpy(pos, patch, sizeof(patch) - 1);
    }
}

void BuildAboutDescription(uint8_t *buffer)
{
    BYTE search_start[] = R"(
<body class="uber-frame">
  <header>
    <h1 i18n-content="aboutTitle"></h1>
)";

    BYTE patch[]  = R"(<body class="uber-frame"><header><h1 i18n-content="aboutTitle"></h1></header><div id="mainview-content"><div id="page-container"><div id="help-page" class="page"><div class="content-area"><div id="about-container"><img id="product-logo" srcset="chrome://theme/current-channel-logo@1x 1x, chrome://theme/current-channel-logo@2x 2x" alt=""><div id="product-description"><h2>Google Chrome With <a href="https://www.shuax.com/?from=greenchrome" target="_blank">GreenChrome</a></h2><span i18n-content="aboutProductDescription"></span></div></div><div id="help-container"><button id="get-help" i18n-content="getHelpWithChrome"></button><button id="report-issue" i18n-content="reportAnIssue"></button></div><div id="version-container"><div i18n-content="browserVersion" dir="ltr"></div><div id="update-status-container" hidden><div id="update-status-icon" class="help-page-icon up-to-date"></div><div id="update-status-message-container"><div id="update-status-message" i18n-content="updateCheckStarted"></div><div id="allowed-connection-types-message" hidden></div></div></div><div id="update-buttons-container"><div id="update-percentage" hidden></div><button id="relaunch" i18n-content="relaunch" hidden></button></div></div><div id="product-container"><div i18n-content="productName"></div><div i18n-content="productCopyright"></div><div id="product-license"></div><div id="product-tos"></div></div></div></div></div></div></body>)";
    size_t patch_size = sizeof(patch) - 1;

    uint8_t* start = memmem(buffer, resources_pak_size, search_start, sizeof(search_start) - 1);
    if(start)
    {
        BYTE search_end[] = R"(</body>)";
        uint8_t* end = memmem(start, resources_pak_size - (start - buffer), search_end, sizeof(search_end) - 1);
        if(end)
        {
            size_t free_size = end + (sizeof(search_end) - 1) - start;
            if ( free_size > patch_size )
            {
                // 打补丁
                memcpy(start, patch, patch_size);

                // 填充空格
                memset(start + patch_size, ' ', free_size - patch_size);
            }
        }
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
            if(MakeNewTabBlank)
            {
                // 让新标签一片空白
                BlankNewTab((BYTE*)buffer);
            }

            // 构造关于描述
            BuildAboutDescription((BYTE*)buffer);
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
        MH_DisableHook(CreateFileW);
    }
    return file;
}

void PatchResourcesPak(const wchar_t *iniPath)
{
    MakeNewTabBlank = GetPrivateProfileInt(L"其它设置", L"新标签空白", 0, iniPath)==1;

    if (MH_CreateHook(CreateFileW, MyCreateFile, (LPVOID*)&RawCreateFile) == MH_OK)
    {
        MH_EnableHook(CreateFileW);
    }
}
