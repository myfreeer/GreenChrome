#define MAX_SIZE 32767

// 如果需要给路径加引号
inline std::wstring QuotePathIfNeeded(const std::wstring &path)
{
    std::vector<wchar_t> buffer(path.length() + 1 /* null */ + 2 /* quotes */);
    wcscpy(&buffer[0], path.c_str());

    PathQuoteSpaces(&buffer[0]);

    return std::wstring(&buffer[0]);
}

// 展开环境路径比如 %windir%
std::wstring ExpandEnvironmentPath(const std::wstring &path)
{
    std::vector<wchar_t> buffer(MAX_PATH);
    size_t expandedLength = ::ExpandEnvironmentStrings(path.c_str(), &buffer[0], (DWORD)buffer.size());
    if (expandedLength > buffer.size())
    {
        buffer.resize(expandedLength);
        expandedLength = ::ExpandEnvironmentStrings(path.c_str(), &buffer[0], (DWORD)buffer.size());
    }
    return std::wstring(&buffer[0], 0, expandedLength);
}

// 替换字符串
void ReplaceStringInPlace(std::wstring& subject, const std::wstring& search, const std::wstring& replace)
{
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::wstring::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

// 运行外部程序
HANDLE RunExecute(const wchar_t *command, WORD show = SW_SHOW)
{
    std::vector <std::wstring> command_line;

    int nArgs;
    LPWSTR *szArglist = CommandLineToArgvW(command, &nArgs);
    for (int i = 0; i < nArgs; i++)
    {
        command_line.push_back(QuotePathIfNeeded(szArglist[i]));
    }
    LocalFree(szArglist);

    SHELLEXECUTEINFO ShExecInfo = { 0 };
    ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
    ShExecInfo.fMask = SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS;
    ShExecInfo.lpFile = command_line[0].c_str();
    ShExecInfo.nShow = show;

    std::wstring parameter;
    for (size_t i = 1; i < command_line.size(); i++)
    {
        parameter += command_line[i];
        parameter += L" ";
    }
    if ( command_line.size() > 1 )
    {
        ShExecInfo.lpParameters = parameter.c_str();
    }

    if (ShellExecuteEx(&ShExecInfo))
    {
        return ShExecInfo.hProcess;
    }
    
    return 0;
}

// 释放配置文件
EXPORT ReleaseIni(const wchar_t *exePath, wchar_t *iniPath)
{
    // ini路径
    wcscpy(iniPath, exePath);
    wcscat(iniPath, L"\\GreenChrome.ini");

    // 生成默认ini文件
    DWORD attribs = GetFileAttributes(iniPath);
    if(attribs == INVALID_FILE_ATTRIBUTES || (attribs & FILE_ATTRIBUTE_DIRECTORY) )
    {
        FILE *fp = _wfopen(iniPath, L"wb");
        if(fp)
        {
            // 从资源中读取默认配置文件
            HRSRC res = FindResource(hInstance, L"CONFIG", L"INI");
            if (res)
            {
                HGLOBAL header = LoadResource(hInstance, res);
                if (header)
                {
                    const char *data = (const char*)LockResource(header);
                    DWORD size = SizeofResource(hInstance, res);
                    if (data)
                    {
                        fwrite(data, size, 1, fp);
                        UnlockResource(header);
                    }
                }
                FreeResource(header);
            }
            fclose(fp);
        }
        else
        {
            MessageBox(0, L"无法释放GreenChrome.ini配置文件，当前目录可能不可写！", L"警告", MB_ICONWARNING);
        }
    }
}

// 搜索内存
uint8_t* memmem(uint8_t* src, int n, const uint8_t* sub, int m)
{
    if (m > n)
    {
        return NULL;
    }

    short skip[256];
    for(int i = 0; i < 256; i++)
    {
        skip[i] = m;
    }
    for(int i = 0; i < m - 1; i++)
    {
        skip[sub[i]] = m - i - 1;
    }


    int pos = 0;
    while(pos <= n - m)
    {
        int j = m -1;
        while(j >= 0 && src[pos + j] == sub[j])
        {
            j--;
        }
        if(j < 0 )
        {
            return src + pos;
        }
        pos = pos + skip[src[pos + m -1]];
    }
    return NULL;
}

bool GetVersion(wchar_t *vinfo)
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    bool ret = false;
    DWORD dummy;
    DWORD infoSize = GetFileVersionInfoSize(exePath, &dummy);
    if (infoSize > 0)
    {
        char *buffer = (char*)malloc(infoSize);

        if (GetFileVersionInfo(exePath, dummy, infoSize, buffer))
        {
            VS_FIXEDFILEINFO *InfoStructure;
            unsigned int TextSize = 0;
            if( VerQueryValue(buffer, L"\\", (VOID **) &InfoStructure, &TextSize) )
            {
                wsprintf(vinfo, L"%d.%d.%d.%d",
                         HIWORD(InfoStructure->dwFileVersionMS), LOWORD(InfoStructure->dwFileVersionMS),
                         HIWORD(InfoStructure->dwFileVersionLS), LOWORD(InfoStructure->dwFileVersionLS));
                ret = true;
            }
        }

        free(buffer);
    }
    return ret;
}

// 搜索模块，自动加载
uint8_t* SearchModule(const wchar_t *path, const uint8_t* sub, int m)
{
    HMODULE module = LoadLibraryW(path);
    
    if(!module)
    {
        // dll存在于版本号文件夹中
        wchar_t version[MAX_PATH];
        GetVersion(version);
        wcscat(version, L"/");
        wcscat(version, path);

        module = LoadLibraryW(version);
    }

    if(module)
    {
        uint8_t* buffer = (uint8_t*)module;

        PIMAGE_NT_HEADERS nt_header = (PIMAGE_NT_HEADERS)(buffer + ((PIMAGE_DOS_HEADER)buffer)->e_lfanew);
        PIMAGE_SECTION_HEADER section = (PIMAGE_SECTION_HEADER)((char*)nt_header + sizeof(DWORD) +
            sizeof(IMAGE_FILE_HEADER) + nt_header->FileHeader.SizeOfOptionalHeader);

        for(int i=0; i<nt_header->FileHeader.NumberOfSections; i++)
        {
            if(strcmp((const char*)section[i].Name,".text")==0)
            {
                return memmem(buffer + section[i].PointerToRawData, section[i].SizeOfRawData, sub, m);
                break;
            }
        }
    }
    return NULL;
}
