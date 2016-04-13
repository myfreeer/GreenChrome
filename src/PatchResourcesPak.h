wchar_t html_file[MAX_PATH];

bool RemoveUpdateError = false;

DWORD resources_pak_size = 0;

#pragma pack(push)
#pragma pack(1)

#define PACK_FILE_VERSION  (4)

struct PAK_HEADER
{
    uint32_t version;
    uint32_t num_entries;
    uint8_t encodeing;
};

struct PAK_ENTRY
{
    uint16_t id;
    uint32_t offset;
};

#pragma pack(pop)

template<typename Function>
void TraversalPakFile(uint8_t *buffer, Function f)
{
    PAK_HEADER *pak_header = (PAK_HEADER*)buffer;

    // 检查文件头
    if (pak_header->version != PACK_FILE_VERSION) return;
    if (pak_header->encodeing != 1) return;

    PAK_ENTRY *pak_entry = (PAK_ENTRY*)(buffer + sizeof(PAK_HEADER));

    // 为了保存最后一条的"下一条"，这条特殊的条目的id一定为0
    PAK_ENTRY *end_entry = pak_entry + pak_header->num_entries;
    if (end_entry->id != 0) return;

    //
    for (uint32_t i = 0; i < pak_header->num_entries; i++)
    {
        PAK_ENTRY *next_entry = pak_entry + 1;
        if (f(buffer + pak_entry->offset, buffer + next_entry->offset, next_entry->offset - pak_entry->offset))
        {
            // 如果不再需要遍历就退出
            break;
        }

        pak_entry = next_entry;
    }
}

void CustomNewTab(uint8_t *buffer)
{
    BYTE search[] = R"(<div id="ntp-contents">)";

    uint8_t* pos = memmem(buffer, resources_pak_size, search, sizeof(search) - 1);
    if(pos)
    {
        TraversalPakFile(buffer, [=](uint8_t *begin, uint8_t* end, uint32_t size){
            if (pos >= begin && pos <= end)
            {
                // 填充空格
                memset(begin, ' ', size);

                if(wcscmp(html_file, L"%demo%")==0)
                {
                    BYTE demo[] = R"(<meta charset="utf-8"><style>html,body{height:100%;overflow:hidden;}body{background-color:#ccc;margin:0;background-image:url(https://unsplash.it/1920/1080?random&blur);background-position:center 0;background-size:cover;}#time{position:fixed;top:50%;left:50%;transform:translate(-50%,-50%);font-size:80px;font-family:'Segoe UI',Arial,'Microsoft Yahei',sans-serif;color:#fff;text-shadow:1px 1px 1px #000;}</style><div id="time">12:00:00</div><script>function p(a){return("0"+a).substr(-2)}function r(){var a=new Date();t=p(a.getHours())+":"+p(a.getMinutes())+":"+p(a.getSeconds());document.getElementById("time").innerText=t}r();setInterval(r,1000);</script>)";
                    size_t demo_size = sizeof(demo) - 1;
                    memcpy(begin, demo, demo_size);
                }

                // 读取文件覆盖内存
                FILE * fp = _wfopen(html_file, L"rb");
                if (fp)
                {
                    fread(begin, size, 1, fp);
                    fclose(fp);
                }
                return true;
            }
            return false;
        });
    }
    else
    {
        DebugLog(L"CustomNewTab failed");
    }
}

void RemovePakUpdateError(uint8_t *buffer)
{
    BYTE search_start[] = R"(container.hidden = status == 'disabled';)";

    BYTE patch[]  = R"(container.hidden = status  = 'disabled';)";
    size_t patch_size = sizeof(patch) - 1;

    uint8_t* start = memmem(buffer, resources_pak_size, search_start, sizeof(search_start) - 1);
    if(start)
    {
        // 打补丁
        memcpy(start, patch, patch_size);
    }
    else
    {
        DebugLog(L"RemovePakUpdateError failed");
    }
}

void BuildAboutDescription(uint8_t *buffer)
{
    BYTE search_start[] = R"(
<body class="uber-frame">
  <header>
    <h1 i18n-content="aboutTitle"></h1>
)";

    BYTE patch[]  = R"(<body class="uber-frame"><header><h1 i18n-content="aboutTitle"></h1></header><div id="mainview-content"><div id="page-container"><div id="help-page" class="page"><div class="content-area"><div id="about-container"><img id="product-logo" srcset="chrome://theme/current-channel-logo@1x 1x, chrome://theme/current-channel-logo@2x 2x" alt=""><div id="product-description"><h2>Google Chrome With <a href="https://www.shuax.com/?from=greenchrome" target="_blank">GreenChrome</a> <small>v)" RELEASE_VER_STR R"(</small></h2><span i18n-content="aboutProductDescription"></span></div></div><div id="help-container"><button id="get-help" i18n-content="getHelpWithChrome"></button><button id="report-issue" i18n-content="reportAnIssue"></button></div><div id="version-container"><div i18n-content="browserVersion" dir="ltr"></div><div id="update-status-container" hidden><div id="update-status-icon" class="help-page-icon up-to-date"></div><div id="update-status-message-container"><div id="update-status-message" i18n-content="updateCheckStarted"></div><div id="allowed-connection-types-message" hidden></div></div></div><div id="update-buttons-container"><div id="update-percentage" hidden></div><button id="relaunch" i18n-content="relaunch" hidden></button></div></div><div id="product-container"><div i18n-content="productName"></div><div i18n-content="productCopyright"></div><div id="product-license"></div><div id="product-tos"></div></div></div></div></div></div></body>)";
    size_t patch_size = sizeof(patch) - 1;

    uint8_t* start = memmem(buffer, resources_pak_size, search_start, sizeof(search_start) - 1);
    if(start)
    {
        BYTE search_end[] = R"(</body>)";
        uint8_t* end = memmem(start, resources_pak_size - (int)(start - buffer), search_end, sizeof(search_end) - 1);
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
    else
    {
        DebugLog(L"BuildAboutDescription failed");
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
            if(html_file[0])
            {
                // 自定义新标签
                CustomNewTab((BYTE*)buffer);
            }

            if(RemoveUpdateError)
            {
                //移除更新错误
                RemovePakUpdateError((BYTE*)buffer);
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
    // 禁止生成debug.log
    if(isEndWith(lpFileName, L"debug.log"))
    {
        return INVALID_HANDLE_VALUE;
    }

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
        //MH_DisableHook(CreateFileW);
    }

    return file;
}

void PatchResourcesPak(const wchar_t *iniPath)
{
    GetPrivateProfileStringW(L"基本设置", L"新标签页面", L"", html_file, MAX_PATH, iniPath);
    RemoveUpdateError = GetPrivateProfileIntW(L"基本设置", L"移除更新错误", 0, iniPath)==1;

    // 扩展环境变量
    std::wstring path = ExpandEnvironmentPath(html_file);

    // exe路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // exe所在文件夹
    wchar_t exeFolder[MAX_PATH];
    wcscpy(exeFolder, exePath);
    PathRemoveFileSpecW(exeFolder);

    // 扩展%app%
    ReplaceStringInPlace(path, L"%app%", exeFolder);

    wcscpy(html_file, path.c_str());

    if(html_file[0])
    {
        // 破解自定义页面不能使用js
        /*
          if (add_content_security_policy_) {
            std::string base = kChromeURLContentSecurityPolicyHeaderBase;
            base.append(RequiresUnsafeEval() ? " 'unsafe-eval'; " : "; ");
            base.append(content_security_policy_object_source_);
            base.append(content_security_policy_frame_source_);
            info->headers->AddHeader(base);
          }
        */
        #ifdef _WIN64
        BYTE search[] = {0x02, 0x00, 0x00, 0x0F, 0x84, 0x88, 0x00, 0x00, 0x00, 0x48};
        uint8_t *unsafe = SearchModule(L"chrome.dll", search, sizeof(search));
        if(unsafe)
        {
            BYTE patch[] = {0x90, 0xE9};
            WriteMemory(unsafe + 3, patch, sizeof(patch));
        }
        else
        {
            search[5] = 0x8D;
            search[9] = 0x45;
            unsafe = SearchModule(L"chrome.dll", search, sizeof(search));
            if (unsafe)
            {
                BYTE patch[] = { 0x90, 0xE9 };
                WriteMemory(unsafe + 3, patch, sizeof(patch));
            }
            else
            {
                DebugLog(L"patch unsafe-js failed");
            }
        }
        #else
        BYTE search[] = {0x01, 0x00, 0x00, 0x00, 0x74, 0x5F, 0x68};
        uint8_t *unsafe = SearchModule(L"chrome.dll", search, sizeof(search));
        if(unsafe)
        {
            BYTE patch[] = {0xEB};
            WriteMemory(unsafe + 4, patch, sizeof(patch));
        }
        else
        {
            DebugLog(L"patch unsafe-js failed");
        }
        #endif
    }

    MH_STATUS status = MH_CreateHook(CreateFileW, MyCreateFile, (LPVOID*)&RawCreateFile);
    if (status == MH_OK)
    {
        MH_EnableHook(CreateFileW);
    }
    else
    {
        DebugLog(L"MH_CreateHook PatchResourcesPak failed:%d", status);
    }
}
