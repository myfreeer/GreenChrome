#include "GreenChrome.h"

void GreenChrome()
{
    // exe路径
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    // exe所在文件夹
    wchar_t exeFolder[MAX_PATH];
    wcscpy(exeFolder, exePath);
    PathRemoveFileSpecW(exeFolder);

    // 生成默认ini文件
    wchar_t iniPath[MAX_PATH];
    ReleaseIni(exeFolder, iniPath);

    // 修复任务栏双图标
    RepairDoubleIcon(iniPath);

    // 修复没有注册类错误
    RepairDelegateExecute(iniPath);

    // 打造便携版chrome
    MakePortable(iniPath);

    // 恢复NPAPI接口
    RecoveryNPAPI(iniPath);

    // 标签页，书签，地址栏增强
    TabBookmark(iniPath);

    // 父进程不是Chrome，则需要启动追加参数功能
    wchar_t parentPath[MAX_PATH];
    if(GetParentPath(parentPath) && _wcsicmp(parentPath, exePath)!=0)
    {
        CustomCommand(iniPath, exeFolder, exePath);
    }
}

EXTERNC BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID pv)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        hInstance = hModule;

        // 保持系统dll原有功能
        LoadSysDll(hModule);

        // 初始化HOOK库成功以后安装加载器
        if (MH_Initialize() == MH_OK)
        {
            InstallLoader();
        }
    }
    return TRUE;
}
