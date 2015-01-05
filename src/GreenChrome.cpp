#define UNICODE
#define _UNICODE

#include "GreenChrome.h"


#define MAX_SIZE 32767

bool DoubleClickCloseTab = true;
bool RightClickCloseTab = true;
bool KeepLeftmostTab = true;
bool FastTabSwitch = true;

HMODULE hInstance;

//自动添加引号
void AppendPath(wchar_t *path, const wchar_t* append)
{
	int len = wcslen(append) + 1 + 2;
	wchar_t *temp = (wchar_t *)malloc(len*sizeof(wchar_t));
	wcscpy(temp, append);

	PathQuoteSpacesW(temp);
	wcscat(path, temp);

	free(temp);
}

HHOOK mouse_hook;
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static bool close_tab_ing = false;
    bool close_tab = false;
    bool keep_tab = false;
    if (nCode==HC_ACTION)
    {
        PMOUSEHOOKSTRUCT pmouse = (PMOUSEHOOKSTRUCT) lParam;
        POINT pt;
        pt.x = pmouse->pt.x;
        pt.y = pmouse->pt.y;
        ScreenToClient(pmouse->hwnd, &pt);

        int close_region[] = {45, 25};
        if(wParam==WM_LBUTTONDBLCLK&&DoubleClickCloseTab)
        {
            if(pt.y<close_region[IsZoomed(pmouse->hwnd)])
            {
                close_tab = true;
            }
        }

        if(wParam==WM_RBUTTONUP&&RightClickCloseTab)
        {
            int hittest = (int)SendMessage(pmouse->hwnd, WM_NCHITTEST, 0, MAKELONG(pmouse->pt.x, pmouse->pt.y));

            if(hittest==HTCLIENT && pt.y<close_region[IsZoomed(pmouse->hwnd)])
            {
                close_tab = true;
            }
        }

        if(close_tab && KeepLeftmostTab)
        {
            int keep_region[] = {204, 212};
            if(pt.x<keep_region[IsZoomed(pmouse->hwnd)])
            {
                keep_tab = true;
            }
        }

        if(wParam==WM_MBUTTONUP)
        {
            if(close_tab_ing)
            {
                close_tab_ing = false;
            }
            else
            {
                if(KeepLeftmostTab)
                {
                    int keep_region[] = {204, 212};
                    if(pt.x<keep_region[IsZoomed(pmouse->hwnd)])
                    {
                        keep_tab = true;
                        close_tab = true;
                    }
                }
            }
        }

        if(wParam==WM_MOUSEWHEEL)
        {
            typedef struct tagMOUSEHOOKSTRUCTEX {
              MOUSEHOOKSTRUCT x;
              DWORD           mouseData;
            } MOUSEHOOKSTRUCTEX, *PMOUSEHOOKSTRUCTEX, *LPMOUSEHOOKSTRUCTEX;
            PMOUSEHOOKSTRUCTEX pwheel = (PMOUSEHOOKSTRUCTEX) lParam;

            if(pt.y<close_region[IsZoomed(pmouse->hwnd)])
            {
                int zDelta = GET_WHEEL_DELTA_WPARAM(pwheel->mouseData);
                if(zDelta>0)
                {
                    INPUT input[2];
                    memset(input, 0, sizeof(input));

                    input[0].type = INPUT_KEYBOARD;
                    input[1].type = INPUT_KEYBOARD;

                    input[0].ki.wVk = VK_CONTROL;
                    input[1].ki.wVk = VK_PRIOR;

                    input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                    input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                    SendInput(2, input, sizeof(INPUT));

                    input[0].ki.dwFlags |= KEYEVENTF_KEYUP;
                    input[1].ki.dwFlags |= KEYEVENTF_KEYUP;
                    SendInput(2, input, sizeof(INPUT));
                }
                else
                {
                    INPUT input[2];
                    memset(input, 0, sizeof(input));

                    input[0].type = INPUT_KEYBOARD;
                    input[1].type = INPUT_KEYBOARD;

                    input[0].ki.wVk = VK_CONTROL;
                    input[1].ki.wVk = VK_NEXT;

                    input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                    input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
                    SendInput(2, input, sizeof(INPUT));

                    input[0].ki.dwFlags |= KEYEVENTF_KEYUP;
                    input[1].ki.dwFlags |= KEYEVENTF_KEYUP;
                    SendInput(2, input, sizeof(INPUT));

                }
                OutputDebugStringA("WM_MOUSEWHEEL");
                return 1;
            }
        }
    }

    if(keep_tab)
    {
        INPUT input[2];
        memset(input, 0, sizeof(input));

        input[0].type = INPUT_KEYBOARD;
        input[1].type = INPUT_KEYBOARD;

        input[0].ki.wVk = VK_CONTROL;
        input[1].ki.wVk = 'T';

        input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
        input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
        SendInput(2, input, sizeof(INPUT));

        input[0].ki.dwFlags |= KEYEVENTF_KEYUP;
        input[1].ki.dwFlags |= KEYEVENTF_KEYUP;
        SendInput(2, input, sizeof(INPUT));
    }

    if(close_tab)
    {
        INPUT input[1];
        memset(input, 0, sizeof(input));

        input[0].type = INPUT_MOUSE;

        input[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        SendInput(1, input, sizeof(INPUT));

        input[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        SendInput(1, input, sizeof(INPUT));

        close_tab_ing = true;
        return 1;
    }

    return CallNextHookEx(mouse_hook, nCode, wParam, lParam );
}

//构造新命令行
void NewCommand(const wchar_t *iniPath,const wchar_t *exePath,const wchar_t *fullPath)
{
	wchar_t *MyCommandLine = (wchar_t *)malloc(MAX_SIZE);
	MyCommandLine[0] = 0;

	int nArgs;
	LPWSTR *szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	for(int i=0; i<nArgs; i++)
	{
		// 原样拼接参数
		if(i!=0)
		{
			wcscat(MyCommandLine, L" ");
		}
		AppendPath(MyCommandLine, szArglist[i]);

		if(i==0) //在进程路径后面追加参数
		{
			wchar_t *temp = (wchar_t *)malloc(MAX_SIZE);

			wchar_t ConfigList[MAX_SIZE];
			GetPrivateProfileSectionW(L"追加参数", ConfigList, MAX_SIZE, iniPath);

			wchar_t *line = ConfigList;
			while (line && *line)
			{
				wchar_t *equ = wcschr(line, '=');
				if(equ)
				{
					//有等号
					*(equ) = 0;
					wcscat(MyCommandLine, L" ");
					wcscat(MyCommandLine, line);
					wcscat(MyCommandLine, L"=");
					*(equ) = '=';

					ExpandEnvironmentStringsW(equ+1, temp, MAX_SIZE);
					wchar_t *_temp = replace(temp, L"%app%", exePath);
					AppendPath(MyCommandLine, _temp);
					free(_temp);
				}
				else
				{
					//无等号
					wcscat(MyCommandLine, L" ");
					AppendPath(MyCommandLine, line);
				}
				line += wcslen(line) + 1;
			}
			free(temp);
		}
	}
	LocalFree(szArglist);

    wchar_t StartProgram[MAX_SIZE];
    GetPrivateProfileSectionW(L"启动时运行", StartProgram, MAX_SIZE, iniPath);

    wchar_t CloseProgram[MAX_SIZE];
    GetPrivateProfileSectionW(L"关闭时运行", CloseProgram, MAX_SIZE, iniPath);

    int first_dll = false;//是否是首先启动的dll
    CreateMutex(NULL, TRUE, L"{56A17F97-9F89-4926-8415-446649F25EB5}");
    if (GetLastError() != ERROR_ALREADY_EXISTS)
    {
    	first_dll = true;
    }

    HANDLE programs[100]; //最多100个外部程序句柄，够用了
    int programs_count = 0; //外部程序数量

    if(first_dll)
    {
        wchar_t *temp = (wchar_t *)malloc(MAX_SIZE);

        wchar_t *line = StartProgram;
        while (line && *line)
        {
            //OutputDebugStringW(line);
            STARTUPINFOW si_ = {0};
            PROCESS_INFORMATION pi_ = {0};
            si_.cb = sizeof(STARTUPINFO);

            ExpandEnvironmentStringsW(line, temp, MAX_SIZE);
            wchar_t *_temp = replace(temp, L"%app%", exePath);

            if (CreateProcessW(NULL, _temp, NULL, NULL, false, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE, NULL, 0, &si_, &pi_))
            {
                programs[programs_count] = pi_.hProcess;
                programs_count++;
                if(programs_count>=100) break;
            }

            free(_temp);

            line += wcslen(line) + 1;
        }
        free(temp);
    }

	//启动进程
	STARTUPINFOW si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(STARTUPINFO);
	//GetStartupInfo(&si);
	//OutputDebugStringW(MyCommandLine);
	if (CreateProcessW(fullPath, MyCommandLine, NULL, NULL, false, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE, NULL, 0, &si, &pi))
	{
	    if(first_dll)
	    {
	        //启动了外部程序时，首个进程不立刻退出，需要检测Chrome的关闭，然后杀掉外部程序
	        //需要结束外部程序，也需要等待处理
            WaitForSingleObject(pi.hProcess, INFINITE);

            //结束附加启动程序
            if(GetPrivateProfileInt(L"其它设置", L"自动关闭", 1, iniPath)==1)
            {
                for(int i=0;i<programs_count;i++)
                {
                    TerminateProcess(programs[i], 0);
                }
            }

            //强制杀掉额外程序
            wchar_t *line = CloseProgram;
            while (line && *line)
            {
                //OutputDebugStringW(line);
                STARTUPINFOW si_ = {0};
                PROCESS_INFORMATION pi_ = {0};
                si_.cb = sizeof(STARTUPINFO);
                si_.dwFlags = STARTF_USESHOWWINDOW;
                si_.wShowWindow = SW_HIDE;

                CreateProcessW(NULL, line, NULL, NULL, false, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT | CREATE_DEFAULT_ERROR_MODE, NULL, 0, &si_, &pi_);


                line += wcslen(line) + 1;
            }
	    }

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		ExitProcess(0);
	}
	free(MyCommandLine);
}

void GreenChrome()
{
	//exe全路径
	wchar_t fullPath[MAX_PATH];
	GetModuleFileNameW(NULL, fullPath, MAX_PATH);

	//exe所在路径
	wchar_t exePath[MAX_PATH];
	wcscpy(exePath, fullPath);
	PathRemoveFileSpecW(exePath);

	//ini路径
	wchar_t iniPath[MAX_PATH];
	wcscpy(iniPath, exePath);
	wcscat(iniPath, L"\\GreenChrome.ini");

	//生成默认ini文件
	if(!PathFileExistsW(iniPath))
	{
		FILE *fp = _wfopen(iniPath, L"wb");
		if(fp)
		{
			fwrite(DefaultConfig, sizeof(DefaultConfig), 1, fp);
			fclose(fp);
		}
	}

	//自动给任务栏pin的快捷方式加上只读属性
	AutoLockLnk(fullPath);

	//修复Win8上常见的没有注册类错误
	FixNoRegisteredClass();

    //
    DoubleClickCloseTab = GetPrivateProfileInt(L"其它设置", L"双击关闭标签", 1, iniPath)==1;
    RightClickCloseTab = GetPrivateProfileInt(L"其它设置", L"右键关闭标签", 1, iniPath)==1;
    KeepLeftmostTab = GetPrivateProfileInt(L"其它设置", L"保留最左标签", 1, iniPath)==1;
    FastTabSwitch = GetPrivateProfileInt(L"其它设置", L"快速标签切换", 1, iniPath)==1;
    if(DoubleClickCloseTab || RightClickCloseTab || KeepLeftmostTab || FastTabSwitch)
    {
        if(!wcsstr(GetCommandLineW(), L"--channel"))
        {
            mouse_hook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, GetCurrentThreadId());
        }
    }

	//父进程不是Chrome，则需要启动GreenChrome功能
	wchar_t parentPath[MAX_PATH];
	if(GetParentPath(parentPath) && _wcsicmp(parentPath, fullPath)!=0)
	{
		//根据配置文件插入额外的命令行参数
		NewCommand(iniPath, exePath, fullPath);
	}
}

EXTERNC BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID pv)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
	    hInstance = hModule;

		//保持系统dll原有功能
		LoadSysDll(hModule);

		//修改程序入口点为 GreenChrome
		InstallLoader();
	}
	return TRUE;
}
