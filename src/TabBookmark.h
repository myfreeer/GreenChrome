

bool DoubleClickCloseTab = false;
bool RightClickCloseTab = false;
bool KeepLastTab = false;
bool FastTabSwitch1 = false;
bool FastTabSwitch2 = false;
bool BookMarkNewTab = false;
bool OpenUrlNewTab = false;
bool NotBlankTab = false;

#ifndef _WIN64
typedef struct tagMOUSEHOOKSTRUCTEX
{
    MOUSEHOOKSTRUCT ignore;
    DWORD           mouseData;
} MOUSEHOOKSTRUCTEX, *PMOUSEHOOKSTRUCTEX, *LPMOUSEHOOKSTRUCTEX;
#endif

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

template<typename Function>
void GetAccessibleName(IAccessible *node, Function f)
{
    VARIANT self;
    self.vt = VT_I4;
    self.lVal = CHILDID_SELF;

    BSTR bstr = NULL;
    if( S_OK == node->get_accHelp(self, &bstr) )
    {
        f(bstr);
        SysFreeString(bstr);
    }
}

template<typename Function>
void GetAccessibleTitle(IAccessible *node, Function f)
{
    VARIANT self;
    self.vt = VT_I4;
    self.lVal = CHILDID_SELF;

    BSTR bstr = NULL;
    if( S_OK == node->get_accName(self, &bstr) )
    {
        f(bstr);
        SysFreeString(bstr);
    }
}

template<typename Function>
void GetAccessibleValue(IAccessible *node, Function f)
{
    VARIANT self;
    self.vt = VT_I4;
    self.lVal = CHILDID_SELF;

    BSTR bstr = NULL;
    if( S_OK == node->get_accValue(self, &bstr) )
    {
        f(bstr);
        SysFreeString(bstr);
    }
}

template<typename Function>
void GetAccessibleSize(IAccessible *node, Function f)
{
    VARIANT self;
    self.vt = VT_I4;
    self.lVal = CHILDID_SELF;

    RECT rect;
    if( S_OK == node->accLocation(&rect.left, &rect.top, &rect.right, &rect.bottom, self) )
    {
        rect.right += rect.left;
        rect.bottom += rect.top;
        f(rect);
    }
}

long GetAccessibleState(IAccessible *node)
{
    VARIANT self;
    self.vt = VT_I4;
    self.lVal = CHILDID_SELF;

    VARIANT state;
    if( S_OK == node->get_accState(self, &state) )
    {
        if (state.vt == VT_I4)
        {
            return state.lVal;
        }
    }
    return 0;
}

template<typename Function>
void TraversalAccessible(IAccessible *node, Function f)
{
    long childCount = 0;
    if( S_OK == node->get_accChildCount(&childCount) && childCount)
    {
        VARIANT* varChildren = (VARIANT*)malloc(sizeof(VARIANT) * childCount);
        if( S_OK == AccessibleChildren(node, 0, childCount, varChildren, &childCount) )
        {
            for(int i=0;i<childCount;i++)
            {
                if( varChildren[i].vt==VT_DISPATCH )
                {
                    IDispatch* dispatch = varChildren[i].pdispVal;
                    IAccessible* child = NULL;
                    if( S_OK == dispatch->QueryInterface(IID_IAccessible, (void**)&child))
                    {
                        if( (GetAccessibleState(child) & STATE_SYSTEM_INVISIBLE) == 0)//可见
                        {
                            if(f(child))
                            {
                                dispatch->Release();
                                break;
                            }
                            child->Release();
                        }
                    }
                    dispatch->Release();
                }
            }
        }
        free(varChildren);
    }
}

IAccessible* FindChildElement(IAccessible *parent, bool aoto_release, const wchar_t *name)
{
    IAccessible* element = NULL;
    if(parent)
    {
        TraversalAccessible(parent, [&element, &name]
            (IAccessible* child){
                GetAccessibleName(child, [&element, &child, &name]
                    (BSTR bstr){
                        if(wcscmp(bstr, name)==0)
                        {
                            element = child;
                        }
                });
                return element!=NULL;
            });
        if(aoto_release) parent->Release();
    }
    return element;
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
    IAccessible *TabStrip = FindChildElement(top, false, L"TabStrip");
    if(TabStrip)
    {
        GetAccessibleSize(TabStrip, [&flag, &pt]
            (RECT rect){
                if(PtInRect(&rect, pt))
                {
                    flag = true;
                }
        });
        TabStrip->Release();
    }
    return flag;
}

//鼠标是否在某个标签上
bool IsOnOneTab(IAccessible* top, POINT pt)
{
    bool flag = false;
    IAccessible *TabStrip = FindChildElement(top, false, L"TabStrip");
    if(TabStrip)
    {
        TraversalAccessible(TabStrip, [&flag, &pt]
            (IAccessible* child){
                GetAccessibleName(child, [&flag, &pt, &child]
                    (BSTR bstr){
                        if(wcscmp(bstr, L"Tab")==0)
                        {
                            GetAccessibleSize(child, [&flag, &pt]
                                (RECT rect){
                                    if(PtInRect(&rect, pt))
                                    {
                                        flag = true;
                                    }
                                });
                        }
                });
                if(flag) child->Release();
                return flag;
            });
        TabStrip->Release();
    }
    return flag;
}

//是否只有一个标签
bool IsOnlyOneTab(IAccessible* top)
{
    IAccessible *TabStrip = FindChildElement(top, false, L"TabStrip");
    if(TabStrip)
    {
        long tab_count = 0;
        TraversalAccessible(TabStrip, [&tab_count]
            (IAccessible* child){
                GetAccessibleName(child, [&tab_count]
                    (BSTR bstr){
                        if(wcscmp(bstr, L"Tab")==0)
                        {
                            tab_count++;
                        }
                });
                return false;
            });
        TabStrip->Release();
        return tab_count<=1;
    }
    return false;
}


bool IsOnOneBookmarkInner(IAccessible* parent, POINT pt)
{
    bool flag = false;
    IAccessible *BookmarkBarView = FindChildElement(parent, false, L"BookmarkBarView");
    if(BookmarkBarView)
    {
        TraversalAccessible(BookmarkBarView, [&flag, &pt]
            (IAccessible* child){
                GetAccessibleName(child, [&flag, &pt, &child]
                    (BSTR bstr){
                        if(wcscmp(bstr, L"BookmarkButton")==0)
                        {
                            GetAccessibleSize(child, [&flag, &pt]
                                (RECT rect){
                                    if(PtInRect(&rect, pt))
                                    {
                                        flag = true;
                                    }
                                });
                        }
                });
                if(flag) child->Release();
                return flag;
            });
        BookmarkBarView->Release();
    }
    return flag;
}

//是否点击书签栏
bool IsOnOneBookmark(IAccessible* top, POINT pt)
{
    bool flag = false;
    if(top)
    {
        //开启了书签栏长显
        if(IsOnOneBookmarkInner(top, pt)) return true;

        //未开启书签栏长显
        IDispatch* dispatch = NULL;
        if( S_OK == top->get_accParent(&dispatch) && dispatch)
        {
            IAccessible* parent = NULL;
            if( S_OK == dispatch->QueryInterface(IID_IAccessible, (void**)&parent))
            {
                flag = IsOnOneBookmarkInner(parent, pt);
                parent->Release();
            }
            dispatch->Release();
        }
    }
    return flag;
}

//当前激活标签是否是新标签
bool IsBlankTab(IAccessible* top)
{
    bool flag = false;
    IAccessible *TabStrip = FindChildElement(top, false, L"TabStrip");
    if(TabStrip)
    {
        wchar_t* new_tab_title = NULL;
        TraversalAccessible(TabStrip, [&new_tab_title]
            (IAccessible* child){
                GetAccessibleName(child, [&child, &new_tab_title]
                    (BSTR bstr){
                        if(wcscmp(bstr, L"ImageButton")==0)
                        {
                            GetAccessibleTitle(child, [&new_tab_title]
                                (BSTR bstr){
                                    new_tab_title = wcsdup(bstr);
                            });
                        }
                });
                return false;
            });
        if(new_tab_title)
        {
            TraversalAccessible(TabStrip, [&flag, &new_tab_title]
                (IAccessible* child){
                    GetAccessibleName(child, [&child, &flag, &new_tab_title]
                        (BSTR bstr){
                            if(wcscmp(bstr, L"Tab")==0)
                            {
                                if(GetAccessibleState(child) & STATE_SYSTEM_SELECTED)
                                {
                                    GetAccessibleTitle(child, [&flag, &new_tab_title]
                                        (BSTR bstr){
                                            if(wcscmp(bstr, new_tab_title)==0)
                                            {
                                                flag = true;
                                            }
                                    });
                                }
                            }
                    });
                    return false;
                });
            free(new_tab_title);
        }
        TabStrip->Release();
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
                    GetAccessibleValue(OmniboxViewViews, [&OmniboxViewViews, &flag]
                        (BSTR bstr){
                            if(bstr[0]!=0)//地址栏不为空
                            {
                                if( (GetAccessibleState(OmniboxViewViews) & STATE_SYSTEM_FOCUSED) == STATE_SYSTEM_FOCUSED)
                                {
                                    flag = true;
                                }
                            }
                    });
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
                if(TopContainerView)
                {
                    TopContainerView->Release();
                }
                return 1;
            }
        }

        if(wParam==WM_LBUTTONUP && BookMarkNewTab && !(GetAsyncKeyState(VK_CONTROL) & KEY_PRESSED) && IsOnOneBookmark(TopContainerView, pmouse->pt) )
        {
            if(!NotBlankTab || !IsBlankTab(TopContainerView))
            {
                bookmark_new_tab = true;
            }
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
            if( !(GetAsyncKeyState(VK_MENU) & KEY_PRESSED) && IsOmniboxViewFocus(TopContainerView) )
            {
                if(!NotBlankTab || !IsBlankTab(TopContainerView))
                {
                    open_url_ing = true;
                }
            }

            if(TopContainerView)
            {
                TopContainerView->Release();
            }

            if(open_url_ing)
            {
                OpenNewUrlTab();
                return 1;
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
    NotBlankTab = GetPrivateProfileInt(L"其它设置", L"非空白页面生效", 0, iniPath)==1;

    if(!wcsstr(GetCommandLineW(), L"--channel"))
    {
        mouse_hook = SetWindowsHookEx(WH_MOUSE, MouseProc, hInstance, GetCurrentThreadId());
        keyboard_hook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, hInstance, GetCurrentThreadId());
    }
}
