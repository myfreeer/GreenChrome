// 构造新命令行
std::wstring GetCommand(const wchar_t *iniPath, const wchar_t *exeFolder)
{
    std::vector <std::wstring> command_line;

    int nArgs;
    LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    for(int i=0; i<nArgs; i++)
    {
        // 保留原来参数
        command_line.push_back( QuotePathIfNeeded(szArglist[i]) );

        if(i==0) //在进程路径后面追加参数
        {
            // 如果需要使用新标签空白，还需要添加一个命令，否则使用的是在线版
            if(GetPrivateProfileInt(L"其它设置", L"新标签空白", 0, iniPath)==1)
            {
                command_line.push_back(L"--force-local-ntp");
            }

            wchar_t additional_parameter[MAX_SIZE];
            GetPrivateProfileSectionW(L"追加参数", additional_parameter, MAX_SIZE, iniPath);

            wchar_t *parameter_ptr = additional_parameter;
            while (parameter_ptr && *parameter_ptr)
            {
                std::wstring parameter_str = parameter_ptr;
                std::size_t equal = parameter_str.find(L"=");
                if (equal != std::wstring::npos)
                {
                    // 含有等号
                    std::wstring parameter = parameter_str.substr(0, equal);

                    // 扩展环境变量
                    std::wstring parameter_path = ExpandEnvironmentPath(parameter_str.substr(equal + 1));

                    // 扩展%app%
                    ReplaceStringInPlace(parameter_path, L"%app%", exeFolder);

                    // 组合参数
                    command_line.push_back( parameter + L"=" + QuotePathIfNeeded(parameter_path) );
                }
                else
                {
                    //添加到参数
                    command_line.push_back( parameter_str );
                }

                parameter_ptr += wcslen(parameter_ptr) + 1;
            }
        }
    }
    LocalFree(szArglist);

    std::wstring my_command_line;
    for(auto str : command_line)
    {
        my_command_line += str;
        my_command_line += L" ";
    }

    return my_command_line;
}


void LaunchAtStart(const wchar_t *iniPath, const wchar_t *exeFolder, std::vector <HANDLE> &program_handles)
{
    wchar_t start_program[MAX_SIZE];
    GetPrivateProfileSectionW(L"启动时运行", start_program, MAX_SIZE, iniPath);

    wchar_t *program_ptr = start_program;
    while (program_ptr && *program_ptr)
    {
        // 扩展环境变量
        std::wstring program = ExpandEnvironmentPath(program_ptr);

        // 扩展%app%
        ReplaceStringInPlace(program, L"%app%", exeFolder);

        // 运行程序
        HANDLE program_handle = RunExecute(program.c_str());
        if (program_handle)
        {
            program_handles.push_back(program_handle);
        }

        program_ptr += wcslen(program_ptr) + 1;
    }
}

void LaunchUpdater(const wchar_t *iniPath, const wchar_t *exeFolder)
{
    // 检查更新
    wchar_t updater_path[MAX_PATH];
    GetPrivateProfileString(L"自动更新", L"更新器地址", L"", updater_path, MAX_PATH, iniPath);
    if(updater_path[0])
    {
        // 扩展环境变量
        std::wstring updater = ExpandEnvironmentPath(updater_path);

        // 扩展%app%
        ReplaceStringInPlace(updater, L"%app%", exeFolder);

        wchar_t check_version[MAX_PATH];
        GetPrivateProfileString(L"自动更新", L"检查版本", L"", check_version, MAX_PATH, iniPath);

        if(check_version[0])
        {
            std::wstring parameters = QuotePathIfNeeded(updater) + L" " + QuotePathIfNeeded(exeFolder) + L" " + check_version;

            // 运行程序
            RunExecute(parameters.c_str());
        }
    }
}

// 退出时运行额外程序
void LaunchAtEnd(const wchar_t *iniPath, const wchar_t *exeFolder)
{
    wchar_t close_program[MAX_SIZE];
    GetPrivateProfileSectionW(L"关闭时运行", close_program, MAX_SIZE, iniPath);

    wchar_t *program_ptr = close_program;
    while (program_ptr && *program_ptr)
    {
        // 扩展环境变量
        std::wstring program = ExpandEnvironmentPath(program_ptr);

        // 扩展%app%
        ReplaceStringInPlace(program, L"%app%", exeFolder);

        // 运行程序
        RunExecute(program.c_str(), SW_HIDE);

        program_ptr += wcslen(program_ptr) + 1;
    }
}

// 结束附加启动程序
void KillAtEnd(const wchar_t *iniPath, std::vector <HANDLE> &program_handles)
{
    if(GetPrivateProfileInt(L"其它设置", L"自动结束运行程序", 0, iniPath)==1)
    {
        for(auto rogram_handle : program_handles)
        {
            TerminateProcess(rogram_handle, 0);
        }
    }
}

// 是否是首先启动的dll
bool IsFirstRun()
{
    bool first_run = false;
    CreateMutex(NULL, TRUE, L"{56A17F97-9F89-4926-8415-446649F25EB5}");
    if (GetLastError() != ERROR_ALREADY_EXISTS)
    {
         first_run = true;
    }

    return first_run;
}

void CustomCommand(const wchar_t *iniPath, const wchar_t *exeFolder, const wchar_t *exePath)
{
    // 只启动一次的功能
    bool first_run = IsFirstRun();
    std::vector <HANDLE> program_handles;
    if(first_run)
    {
        // 启动老板键
        Bosskey(iniPath);

        // 启动更新器
        LaunchUpdater(iniPath, exeFolder);

        // 启动时运行
        LaunchAtStart(iniPath, exeFolder, program_handles);
    }

    // 启动进程
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi = {0};
    si.cb = sizeof(STARTUPINFO);

    // 根据配置文件插入额外的命令行参数
    std::wstring my_command_line = GetCommand(iniPath, exeFolder);

    if (CreateProcessW(exePath, (LPWSTR)my_command_line.c_str(), NULL, NULL, false, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE, NULL, 0, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);

        // 结束时杀掉启动时运行的程序
        KillAtEnd(iniPath, program_handles);

        // 结束时运行
        LaunchAtEnd(iniPath, exeFolder);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        ExitProcess(0);
    }
}