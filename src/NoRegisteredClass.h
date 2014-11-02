
void FixNoRegisteredClass()
{
    HKEY hKey;
    if(RegOpenKeyEx(HKEY_CLASSES_ROOT, _T(".html"), 0, KEY_QUERY_VALUE, &hKey)==ERROR_SUCCESS)
    {
        TCHAR html[MAX_PATH];
        DWORD dwLength = MAX_PATH;
        if(RegQueryValueEx(hKey, NULL, NULL, NULL, (LPBYTE)html, &dwLength)==ERROR_SUCCESS)
        {
            RegCloseKey(hKey);

            if (_tcsstr(html, _T("ChromeHTML")) != 0)
            {
                //当前关联给chrome
                TCHAR buffer[MAX_PATH];//ChromeHTML.ULBBU7VXRLOIAV7H45MHGMEO3U\shell\open\command
                wsprintf(buffer, _T("%s\\shell\\open\\command"), html);

                HKEY hRegKey;
                if(RegOpenKeyEx(HKEY_CLASSES_ROOT, buffer, 0, KEY_ALL_ACCESS, &hRegKey)==ERROR_SUCCESS)
                {
                    if(RegDeleteValue(hRegKey, _T("DelegateExecute"))==ERROR_SUCCESS)
                    {
                        //OutputDebugStringW(L"ok");
                    }
                    RegCloseKey(hRegKey);
                }
            }
        }
        else
        {
            RegCloseKey(hKey);
        }
    }
}
