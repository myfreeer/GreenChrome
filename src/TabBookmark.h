

bool DoubleClickCloseTab = false;
bool RightClickCloseTab = false;
bool KeepLastTab = false;
bool FastTabSwitch1 = false;
bool FastTabSwitch2 = false;
bool BookMarkNewTab = false;
bool OpenUrlNewTab = false;

typedef struct tagMOUSEHOOKSTRUCTEX
{
    MOUSEHOOKSTRUCT ignore;
    DWORD           mouseData;
} MOUSEHOOKSTRUCTEX, *PMOUSEHOOKSTRUCTEX, *LPMOUSEHOOKSTRUCTEX;

#define KEY_PRESSED 0x8000


void SendMiddleClick()
{
    INPUT input[1];
    memset(input, 0, sizeof(input));

    input[0].type = INPUT_MOUSE;

    input[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
    SendInput(1, input, sizeof(INPUT));

    input[0].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
    SendInput(1, input, sizeof(INPUT));
}

void OpenNewTab()
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

void SwitchTab(int zDelta)
{
    INPUT input[2];
    memset(input, 0, sizeof(input));

    input[0].type = INPUT_KEYBOARD;
    input[1].type = INPUT_KEYBOARD;

    input[0].ki.wVk = VK_CONTROL;
    input[1].ki.wVk = (zDelta>0)?VK_PRIOR:VK_NEXT;

    input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    SendInput(2, input, sizeof(INPUT));

    input[0].ki.dwFlags |= KEYEVENTF_KEYUP;
    input[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(2, input, sizeof(INPUT));
}

void OpenNewUrlTab()
{
    INPUT input[2];
    memset(input, 0, sizeof(input));

    input[0].type = INPUT_KEYBOARD;
    input[1].type = INPUT_KEYBOARD;

    input[0].ki.wVk = VK_MENU;
    input[1].ki.wVk = VK_RETURN;

    input[0].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    input[1].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    SendInput(2, input, sizeof(INPUT));

    input[0].ki.dwFlags |= KEYEVENTF_KEYUP;
    input[1].ki.dwFlags |= KEYEVENTF_KEYUP;
    SendInput(2, input, sizeof(INPUT));
}
/*
browser/ui/views/frame/BrowserRootView
ui/views/window/NonClientView
BrowserView
TopContainerView
    TabStrip
        Tab
            TabCloseButton
        ImageButton
    ToolbarView
        LocationBarView
            OmniboxViewViews
    BookmarkBarView
        BookmarkButton
*/

IAccessible* FindChildElement(IAccessible *parent, bool aoto_release, const wchar_t *name)
{
    long childCount = 0;
    if( S_OK == parent->get_accChildCount(&childCount) && childCount)
    {
        VARIANT* varChildren = (VARIANT*)malloc(sizeof(VARIANT) * childCount);
        if( S_OK == AccessibleChildren(parent, 0, childCount, varChildren, &childCount) )
        {
            for(int i=0;i<childCount;i++)
            {
                if( varChildren[i].vt==VT_DISPATCH )
                {
                    IDispatch* dispatch = varChildren[i].pdispVal;
                    IAccessible* child = NULL;
                    if( S_OK == dispatch->QueryInterface(IID_IAccessible, (void**)&child))
                    {
                        VARIANT self;
                        self.vt = VT_I4;
                        self.lVal = CHILDID_SELF;

                        BSTR bstrName = NULL;
                        if( S_OK == child->get_accHelp(self, &bstrName) )
                        {
                            if(wcscmp(bstrName, name)==0)
                            {
                                SysFreeString(bstrName);
                                dispatch->Release();
                                if(aoto_release) parent->Release();
                                return child;
                            }
                            SysFreeString(bstrName);
                        }
                        child->Release();
                    }
                    dispatch->Release();
                }
            }
        }
        free(varChildren);
    }
    if(aoto_release) parent->Release();
    return NULL;
}

IAccessible* FindTopContainerView(IAccessible *top)
{
    top = FindChildElement(top, true, L"browser/ui/views/frame/BrowserRootView");
    if(!top) return NULL;

    top = FindChildElement(top, true, L"ui/views/window/NonClientView");
    if(!top) return NULL;

    top = FindChildElement(top, true, L"BrowserView");
    if(!top) return NULL;

    top = FindChildElement(top, true, L"TopContainerView");
    return top;
}

IAccessible* GetTopContainerView(HWND hwnd)
{
    IAccessible *TopContainerView = NULL;
    wchar_t name[256];
    GetClassName(hwnd, name, 256);
    if(wcscmp(name, L"Chrome_WidgetWin_1")==0)
    {
        IAccessible *paccMainWindow = NULL;
        if ( S_OK == AccessibleObjectFromWindow(hwnd, OBJID_WINDOW, IID_IAccessible, (void**)&paccMainWindow) )
        {
            TopContainerView = FindTopContainerView(paccMainWindow);
        }
    }
    return TopContainerView;
}

//鼠标是否在标签栏上
bool IsOnTheTab(IAccessible* top, POINT pt)
{
    bool flag = false;
    if(top)
    {
        IAccessible *TabStrip = FindChildElement(top, false, L"TabStrip");
        if(TabStrip)
        {
            VARIANT self;
            self.vt = VT_I4;
            self.lVal = CHILDID_SELF;
            RECT rect;
            if( S_OK == TabStrip->accLocation(&rect.left, &rect.top, &rect.right, &rect.bottom, self))
            {
                rect.right += rect.left;
                rect.bottom += rect.top;

                if(PtInRect(&rect, pt))
                {
                    flag = true;
                }
            }
            TabStrip->Release();
        }
    }
    return flag;
}

//鼠标是否在某个标签上
bool IsOnOneTab(IAccessible* top, POINT pt)
{
    bool flag = false;
    if(top)
    {
        IAccessible *TabStrip = FindChildElement(top, false, L"TabStrip");
        if(TabStrip)
        {
            long childCount = 0;
            if( S_OK == TabStrip->get_accChildCount(&childCount) && childCount)
            {
                VARIANT* varChildren = (VARIANT*)malloc(sizeof(VARIANT) * childCount);
                if( S_OK == AccessibleChildren(TabStrip, 0, childCount, varChildren, &childCount) )
                {
                    for(int i=0; i<childCount; i++)
                    {
                        if( varChildren[i].vt==VT_DISPATCH )
                        {
                            IDispatch* dispatch = varChildren[i].pdispVal;
                            IAccessible* child = NULL;
                            if( S_OK == dispatch->QueryInterface(IID_IAccessible, (void**)&child))
                            {
                                VARIANT self;
                                self.vt = VT_I4;
                                self.lVal = CHILDID_SELF;

                                BSTR bstrName = NULL;
                                if( S_OK == child->get_accHelp(self, &bstrName) )
                                {
                                    if(wcscmp(bstrName, L"Tab")==0)
                                    {
                                        RECT rect;
                                        if( S_OK == child->accLocation(&rect.left, &rect.top, &rect.right, &rect.bottom, self))
                                        {
                                            rect.right += rect.left;
                                            rect.bottom += rect.top;

                                            if(PtInRect(&rect, pt))
                                            {
                                                flag = true;
                                            }
                                        }
                                    }
                                    SysFreeString(bstrName);
                                }
                                child->Release();
                            }
                            dispatch->Release();
                        }
                    }
                }
                free(varChildren);
            }
            TabStrip->Release();
        }

    }
    return flag;
}

//是否只有一个标签
bool IsOnlyOneTab(IAccessible* top)
{
    if(top)
    {
        IAccessible *TabStrip = FindChildElement(top, false, L"TabStrip");
        if(TabStrip)
        {
            long tab_count = 0;;
            long childCount = 0;
            if( S_OK == TabStrip->get_accChildCount(&childCount) && childCount )
            {
                VARIANT* varChildren = (VARIANT*)malloc(sizeof(VARIANT) * childCount);
                if( S_OK == AccessibleChildren(TabStrip, 0, childCount, varChildren, &childCount) )
                {
                    for(int i=0; i<childCount; i++)
                    {
                        if( varChildren[i].vt==VT_DISPATCH )
                        {
                            IDispatch* dispatch = varChildren[i].pdispVal;
                            IAccessible* child = NULL;
                            if( S_OK == dispatch->QueryInterface(IID_IAccessible, (void**)&child))
                            {
                                VARIANT self;
                                self.vt = VT_I4;
                                self.lVal = CHILDID_SELF;

                                BSTR bstrName = NULL;
                                if( S_OK == child->get_accHelp(self, &bstrName) )
                                {
                                    if(wcscmp(bstrName, L"Tab")==0)
                                    {
                                        tab_count++;
                                    }
                                    SysFreeString(bstrName);
                                }
                                child->Release();
                            }
                            dispatch->Release();
                        }
                    }
                }
                free(varChildren);
            }
            TabStrip->Release();
            return tab_count<=1;
        }
    }
    return false;
}

//是否点击书签栏
bool IsOnOneBookmark(IAccessible* top, POINT pt)
{
    bool flag = false;
    if(top)
    {
        IAccessible *BookmarkBarView = FindChildElement(top, false, L"BookmarkBarView");
        if(BookmarkBarView)
        {
            long childCount = 0;
            if( S_OK == BookmarkBarView->get_accChildCount(&childCount) && childCount)
            {
                VARIANT* varChildren = (VARIANT*)malloc(sizeof(VARIANT) * childCount);
                if( S_OK == AccessibleChildren(BookmarkBarView, 0, childCount, varChildren, &childCount) )
                {
                    for(int i=0; i<childCount; i++)
                    {
                        if( varChildren[i].vt==VT_DISPATCH )
                        {
                            IDispatch* dispatch = varChildren[i].pdispVal;
                            IAccessible* child = NULL;
                            if( S_OK == dispatch->QueryInterface(IID_IAccessible, (void**)&child))
                            {
                                VARIANT self;
                                self.vt = VT_I4;
                                self.lVal = CHILDID_SELF;

                                BSTR bstrName = NULL;
                                if( S_OK == child->get_accHelp(self, &bstrName) )
                                {
                                    if(wcscmp(bstrName, L"BookmarkButton")==0)
                                    {
                                        RECT rect;
                                        if( S_OK == child->accLocation(&rect.left, &rect.top, &rect.right, &rect.bottom, self))
                                        {
                                            rect.right += rect.left;
                                            rect.bottom += rect.top;

                                            if(PtInRect(&rect, pt))
                                            {
                                                flag = true;
                                            }
                                        }
                                    }
                                    SysFreeString(bstrName);
                                }
                                child->Release();
                            }
                            dispatch->Release();
                        }
                    }
                }
                free(varChildren);
            }
            BookmarkBarView->Release();
        }

    }
    return flag;
}

bool IsOmniboxViewFocus(IAccessible* top)
{
    bool flag = false;
    if(top)
    {
        IAccessible *ToolbarView = FindChildElement(top, false, L"ToolbarView");
        if(ToolbarView)
        {
            IAccessible *LocationBarView = FindChildElement(ToolbarView, true, L"LocationBarView");
            if(LocationBarView)
            {
                IAccessible *OmniboxViewViews = FindChildElement(LocationBarView, true, L"OmniboxViewViews");
                if(OmniboxViewViews)
                {
                    VARIANT self;
                    self.vt = VT_I4;
                    self.lVal = CHILDID_SELF;

                    BSTR bstrName = NULL;
                    if( S_OK == OmniboxViewViews->get_accValue(self, &bstrName) )
                    {
                        if(bstrName[0]!=0)//地址栏不为空
                        {
                            VARIANT varRetVal;
                            OmniboxViewViews->get_accState(self, &varRetVal);
                            if (varRetVal.vt == VT_I4)
                            {
                                if( (varRetVal.lVal & STATE_SYSTEM_FOCUSED) == STATE_SYSTEM_FOCUSED)
                                {
                                    flag = true;
                                }
                            }
                        }
                        SysFreeString(bstrName);
                    }
                    OmniboxViewViews->Release();
                }
            }

        }
    }
    return flag;
}

HHOOK mouse_hook;
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static bool close_tab_ing = false;
    static bool wheel_tab_ing = false;

    bool close_tab = false;
    bool keep_tab = false;
    bool bookmark_new_tab = false;

    if (nCode==HC_ACTION)
    {
        PMOUSEHOOKSTRUCT pmouse = (PMOUSEHOOKSTRUCT) lParam;

        if(wParam==WM_MOUSEMOVE)
        {
            return CallNextHookEx(mouse_hook, nCode, wParam, lParam );;
        }

        if(wParam==WM_RBUTTONUP && wheel_tab_ing)
        {
            wheel_tab_ing = false;
            return 1;
        }

        IAccessible* TopContainerView = GetTopContainerView(pmouse->hwnd);

        if(wParam==WM_LBUTTONDBLCLK && DoubleClickCloseTab && IsOnOneTab(TopContainerView, pmouse->pt))
        {
            close_tab = true;
        }

        if(wParam==WM_RBUTTONUP && RightClickCloseTab && IsOnOneTab(TopContainerView, pmouse->pt))
        {
            close_tab = true;
        }

        if(close_tab && KeepLastTab && IsOnlyOneTab(TopContainerView))
        {
            keep_tab = true;
        }

        if(wParam==WM_MBUTTONUP && IsOnOneTab(TopContainerView, pmouse->pt))
        {
            if(close_tab_ing)
            {
                close_tab_ing = false;
            }
            else
            {
                if(KeepLastTab && IsOnlyOneTab(TopContainerView))
                {
                    keep_tab = true;
                    close_tab = true;
                }
            }
        }

        if(wParam==WM_MOUSEWHEEL)
        {
            PMOUSEHOOKSTRUCTEX pwheel = (PMOUSEHOOKSTRUCTEX) lParam;
            int zDelta = GET_WHEEL_DELTA_WPARAM(pwheel->mouseData);
            if( FastTabSwitch1 && IsOnTheTab(TopContainerView, pmouse->pt) )
            {
                SwitchTab(zDelta);
            }
            else if( FastTabSwitch2 && (GetAsyncKeyState(VK_RBUTTON) & KEY_PRESSED) )
            {
                SwitchTab(zDelta);
                wheel_tab_ing = true;
                return 1;
            }
        }

        if(wParam==WM_LBUTTONUP && BookMarkNewTab && IsOnOneBookmark(TopContainerView, pmouse->pt) && !(GetAsyncKeyState(VK_CONTROL) & KEY_PRESSED) )
        {
            bookmark_new_tab = true;
        }

        if(TopContainerView)
        {
            TopContainerView->Release();
        }
    }

    if(keep_tab)
    {
        OpenNewTab();
    }

    if(close_tab)
    {
        SendMiddleClick();
        close_tab_ing = true;
        return 1;
    }

    if(bookmark_new_tab)
    {
        SendMiddleClick();
        return 1;
    }

    return CallNextHookEx(mouse_hook, nCode, wParam, lParam );
}


HHOOK keyboard_hook;
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    static bool open_url_ing = false;
    if (nCode==HC_ACTION)
    {
        if(wParam==VK_RETURN && OpenUrlNewTab)
        {
            if(open_url_ing)
            {
                open_url_ing = false;
                return CallNextHookEx(keyboard_hook, nCode, wParam, lParam );
            }

            IAccessible* TopContainerView = GetTopContainerView(GetForegroundWindow());
            if(IsOmniboxViewFocus(TopContainerView) && !(GetAsyncKeyState(VK_MENU) & KEY_PRESSED))
            {
                open_url_ing = true;
                OpenNewUrlTab();
                return 1;
            }
            if(TopContainerView)
            {
                TopContainerView->Release();
            }
        }
    }
    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam );
}

void TabBookmark(HMODULE hInstance, const wchar_t *iniPath)
{
    DoubleClickCloseTab = GetPrivateProfileInt(L"其它设置", L"双击关闭标签", 0, iniPath)==1;
    RightClickCloseTab = GetPrivateProfileInt(L"其它设置", L"右键关闭标签", 0, iniPath)==1;
    KeepLastTab = GetPrivateProfileInt(L"其它设置", L"保留最后标签", 0, iniPath)==1;
    FastTabSwitch1 = GetPrivateProfileInt(L"其它设置", L"快速标签切换1", 0, iniPath)==1;
    FastTabSwitch2 = GetPrivateProfileInt(L"其它设置", L"快速标签切换2", 0, iniPath)==1;
    BookMarkNewTab = GetPrivateProfileInt(L"其它设置", L"新标签打开书签", 0, iniPath)==1;
    OpenUrlNewTab = GetPrivateProfileInt(L"其它设置", L"新标签打开网址", 0, iniPath)==1;

    if(!wcsstr(GetCommandLineW(), L"--channel"))
    {
        mouse_hook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, GetCurrentThreadId());
        keyboard_hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInstance, GetCurrentThreadId());
    }
}
