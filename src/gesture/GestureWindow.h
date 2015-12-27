#include "LayeredWindowUtils.h"

#define WM_USER_HWND (WM_USER + 1)
#define WM_USER_SHOW (WM_USER + 2)
#define WM_USER_UPDATE (WM_USER + 3)
#define WM_USER_END (WM_USER + 4)

class GestureWindow :
    public ATL::CWindowImpl<GestureWindow>,
    public LayeredWindowUtils<GestureWindow>
{
public:
    DECLARE_WND_CLASS(L"GreenChromeGesture")

    BEGIN_MSG_MAP(GestureWindow)
        MSG_WM_CREATE(OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_USER_HWND, OnUserHwnd)
        MESSAGE_HANDLER(WM_USER_SHOW, OnUserShow)
        MESSAGE_HANDLER(WM_USER_UPDATE, Update)
        MESSAGE_HANDLER(WM_USER_END, End)
    END_MSG_MAP()

    void DrawGestureTrack(Graphics &graphics)
    {
        Pen pen(Color(200, 152, 204, 0), 2.5);

        std::vector<POINT> points = gesture_recognition.get_points();
        if(points.size()<2) return;

        ScreenToClient(&points[0]);
        POINT last_point = points[0];
        for (size_t i = 1; i < points.size(); ++i)
        {
            if( gesture_recognition.GetDistance(last_point, points[i]) > 4 )
            {
                ScreenToClient(&points[i]);
                graphics.DrawLine(&pen, last_point.x, last_point.y, points[i].x, points[i].y);
                last_point = points[i];
            }
        }
    }

    void DrawGestureResult(Graphics &graphics)
    {
        std::wstring command = gesture_recognition.result();
        if (command.empty()) return;
        if(!image_up) return;

        RECT rect;
        ::GetWindowRect(main_hwnd, &rect);

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;

        size_t max_count = width / image_up->GetWidth();
        size_t count = min(max_count, command.length());

        int x = rect.left + (width - image_up->GetWidth() * (int)count) / 2;

        int y = rect.bottom - 200;
        for (size_t i = 0; i < count; ++i)
        {
            switch (command[i])
            {
            case L'↑':
                graphics.DrawImage(image_up, x, y, image_up->GetWidth(), image_up->GetHeight());
                break;
            case L'→':
                graphics.DrawImage(image_right, x, y, image_right->GetWidth(), image_right->GetHeight());
                break;
            case L'↓':
                graphics.DrawImage(image_down, x, y, image_down->GetWidth(), image_down->GetHeight());
                break;
            case L'←':
                graphics.DrawImage(image_left, x, y, image_left->GetWidth(), image_left->GetHeight());
                break;

            case L'↗':
                graphics.DrawImage(image_topright, x, y, image_topright->GetWidth(), image_topright->GetHeight());
                break;
            case L'↘':
                graphics.DrawImage(image_down_right, x, y, image_down_right->GetWidth(), image_down_right->GetHeight());
                break;
            case L'↙':
                graphics.DrawImage(image_down_left, x, y, image_down_left->GetWidth(), image_down_left->GetHeight());
                break;
            case L'↖':
                graphics.DrawImage(image_topleft, x, y, image_topleft->GetWidth(), image_topleft->GetHeight());
                break;
            }

            x += image_up->GetWidth();
        }

        FontFamily fontFamily(font_name.c_str());
        Font font(&fontFamily, 24, FontStyleRegular, UnitPixel);

        StringFormat stringformat;
        stringformat.SetAlignment(StringAlignmentCenter);
        stringformat.SetLineAlignment(StringAlignmentCenter);

        SolidBrush brush(Color(255, 0xa9, 0xa9, 0xa9));
        std::wstring command_name = GetGestureName(command);

        graphics.DrawString(command_name.c_str(), -1, &font,
            RectF((float)rect.left, (float)rect.bottom - 200, (float)width, (float)160), &stringformat, &brush);
    }

    void DoLayeredPaint(WTL::CDCHandle dc, RECT rcclient)
    {
        Graphics graphics(dc);
        graphics.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        //graphics.Clear(0x99345678);

        DrawGestureTrack(graphics);
        DrawGestureResult(graphics);
    }

    LRESULT OnUserHwnd(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        main_hwnd = (HWND)wParam;
        return 0;
    }

    LRESULT OnUserShow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        ::SetWindowPos(m_hWnd, NULL, 0, 0, ::GetSystemMetrics(SM_CXSCREEN), ::GetSystemMetrics(SM_CYSCREEN), SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        CenterWindow();

        DoUpdateWindow();

        ShowWindow(SW_SHOW);
        return 0;
    }

    LRESULT Update(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        DoUpdateWindow();
        return 0;
    }

    LRESULT End(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        std::wstring command = (wchar_t*)wParam;
        ShowWindow(SW_HIDE);
        if (command.empty()) return 0;

        std::wstring action = GetAction(command);
        if (action.empty()) return 0;

        SendKey(action);
        return 0;
    }

private:
    LRESULT OnCreate(LPCREATESTRUCT lpCreateStruct)
    {
        ImageFromIDResource(L"up", image_up);
        ImageFromIDResource(L"down", image_down);
        ImageFromIDResource(L"left", image_left);
        ImageFromIDResource(L"right", image_right);
        ImageFromIDResource(L"topleft", image_topleft);
        ImageFromIDResource(L"topright", image_topright);
        ImageFromIDResource(L"down_left", image_down_left);
        ImageFromIDResource(L"down_right", image_down_right);

        LOGFONT lf = { 0 };
        NONCLIENTMETRICS ncm = { sizeof(NONCLIENTMETRICS) };
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, false);
        font_name = ncm.lfMessageFont.lfFaceName;


        GetModuleFileNameW(NULL, ini_path, MAX_PATH);
        PathRemoveFileSpecW(ini_path);
        wcscat(ini_path, L"\\GreenChrome.ini");

        return 0;
    }

    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        ::PostQuitMessage(0);
        return 0;
    }

    std::wstring GetAction(std::wstring &command)
    {
        wchar_t buffer[256];
        GetPrivateProfileString(L"鼠标手势", command.c_str(), L"", buffer, 256, ini_path);
        if(buffer[0])
        {
            std::wstring action = buffer;
            std::size_t pos = action.find(L"|");
            if (pos != std::wstring::npos)
            {
                return action.substr(pos + 1);
            }
        }

        return L"";
    }

    std::wstring GetGestureName(std::wstring &command)
    {
        wchar_t buffer[256];
        GetPrivateProfileString(L"鼠标手势", command.c_str(), L"", buffer, 256, ini_path);
        if(buffer[0])
        {
            std::wstring action = buffer;
            std::size_t pos = action.find(L"|");
            if (pos != std::wstring::npos)
            {
                return action.substr(0, pos);
            }
        }
        else
        {
            return L"无动作";
        }

        return L"";
    }
private:
    Image *image_up;
    Image *image_down;
    Image *image_left;
    Image *image_right;
    Image *image_topleft;
    Image *image_topright;
    Image *image_down_left;
    Image *image_down_right;
    std::wstring font_name;
    HWND main_hwnd;
    wchar_t ini_path[MAX_PATH];
};