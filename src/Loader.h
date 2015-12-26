
typedef int (*Startup) ();
Startup ChromeMain = NULL;

// 
int Loader()
{
    GreenChrome();

    //返回到Chrome
    return ChromeMain();
}

void InstallLoader()
{
    //获取程序入口点
    MODULEINFO mi;
    GetModuleInformation(GetCurrentProcess(), GetModuleHandle(NULL), &mi, sizeof(MODULEINFO));
    PBYTE entry = (PBYTE)mi.EntryPoint;

    // 入口点跳转到Loader
    if (MH_CreateHook(entry, Loader, (LPVOID*)&ChromeMain) == MH_OK)
    {
        MH_EnableHook(entry);
    }
}
