#pragma data_seg(".SHARED")

bool DoubleClickCloseTab = false;
bool RightClickCloseTab = false;
bool KeepLastTab = false;
bool HoverTabSwitch = false;
bool RightTabSwitch = false;
bool BookMarkNewTab = false;
bool OpenUrlNewTab = false;
bool NotBlankTab = false;
bool FrontNewTab = false;
bool MouseGesture = false;
bool StopWeb = false;

#pragma data_seg()
#pragma comment(linker, "/section:.SHARED,RWS")

void ReadConfig(const wchar_t *iniPath)
{
    DoubleClickCloseTab = GetPrivateProfileInt(L"������ǿ", L"˫���رձ�ǩҳ", 1, iniPath) == 1;
    RightClickCloseTab = GetPrivateProfileInt(L"������ǿ", L"�Ҽ��رձ�ǩҳ", 0, iniPath) == 1;
    KeepLastTab = GetPrivateProfileInt(L"������ǿ", L"��������ǩ", 1, iniPath) == 1;
    HoverTabSwitch = GetPrivateProfileInt(L"������ǿ", L"��ͣ���ٱ�ǩ�л�", 1, iniPath) == 1;
    RightTabSwitch = GetPrivateProfileInt(L"������ǿ", L"�Ҽ����ٱ�ǩ�л�", 1, iniPath) == 1;
    BookMarkNewTab = GetPrivateProfileInt(L"������ǿ", L"�±�ǩ����ǩ", 1, iniPath) == 1;
    OpenUrlNewTab = GetPrivateProfileInt(L"������ǿ", L"�±�ǩ����ַ", 0, iniPath) == 1;
    NotBlankTab = GetPrivateProfileInt(L"������ǿ", L"�±�ǩҳ����Ч", 1, iniPath) == 1;
    FrontNewTab = GetPrivateProfileInt(L"������ǿ", L"ǰ̨���±�ǩ", 1, iniPath) == 1;

    MouseGesture = GetPrivateProfileInt(L"�������", L"����", 1, iniPath) == 1;

    StopWeb = GetPrivateProfileInt(L"��������", L"ͣ��WEB����", 0, iniPath) == 1;
}