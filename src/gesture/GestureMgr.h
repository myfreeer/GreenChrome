#include <mutex>
#include <chrono>
#include <thread>
#include <condition_variable>

// 识别是否启用手势
class GestureMgr
{
public:
    void Init(GestureWindow *wnd)
    {
        gesture_window = wnd;
        running_ = false;
        recognition_ = false;
        abort_ = false;
        ignore_mouse_event = false;
        manager_thread_ = new std::thread(&GestureMgr::Thread, this);
    }

    void Exit()
    {
        abort_ = true;
        mouse_down_event_.notify_one();
        mouse_up_event_.notify_one();
        manager_thread_->join();
        delete manager_thread_;
    }

    //鼠标右键按下
    bool OnRButtonDown(PMOUSEHOOKSTRUCT pmouse)
    {
        if(!ignore_mouse_event)
        {
            running_ = true;
            gesture_recognition.init(pmouse->pt.x, pmouse->pt.y);
            mouse_down_event_.notify_one();
            gesture_window->SendMessageW(WM_USER_HWND, (WPARAM)pmouse->hwnd);
            return true;
        }

        return false;
    }

    //鼠标右键弹起
    bool OnRButtonUp(PMOUSEHOOKSTRUCT pmouse)
    {
        if(!ignore_mouse_event && running_)
        {
            if(recognition_)
            {
                std::wstring command = gesture_recognition.result();

                gesture_window->SendMessageW(WM_USER_END, (WPARAM)command.c_str());
            }
            mouse_up_event_.notify_one();
            running_ = false;
            recognition_ = false;
            return true;
        }

        ignore_mouse_event = false;
        return false;
    }

    //鼠标移动
    void Move(int x, int y)
    {
        gesture_recognition.move(x, y);
        gesture_window->SendMessageW(WM_USER_UPDATE);
    }

    //是否进入识别状态
    bool IsRunning()
    {
        return running_;
    }
private:
    void Thread()
    {
        while(!abort_)
        {
            std::unique_lock<std::mutex> event_lock(event_mutex_);

            mouse_down_event_.wait(event_lock);
            if(abort_) break;

            auto now = std::chrono::high_resolution_clock::now();

            std::cv_status result = mouse_up_event_.wait_for(event_lock, std::chrono::milliseconds(255));
            //使用返回值判断std::cv_status::timeout似乎不准，所以这里多加5ms
            if(abort_) break;

            auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - now).count();
            if(diff > 150)
            {
                recognition_ = true;

                gesture_window->SendMessageW(WM_USER_SHOW);
            }
            else
            {
                ignore_mouse_event = true;
                SendOneMouse(MOUSEEVENTF_RIGHTDOWN);
                SendOneMouse(MOUSEEVENTF_RIGHTUP);
            }
        }
    }
	void SendOneMouse(int mouse)
	{
		INPUT input[1];
		memset(input, 0, sizeof(input));

		input[0].type = INPUT_MOUSE;

		input[0].mi.dwFlags = mouse;
		::SendInput(1, input, sizeof(INPUT));
	}
private:
    bool running_;
    bool recognition_;
    bool ignore_mouse_event;
    bool abort_;

    std::thread *manager_thread_;

    std::mutex event_mutex_;
    std::condition_variable mouse_down_event_;
    std::condition_variable mouse_up_event_;
    GestureWindow *gesture_window;
} gesture_mgr;

void StartGestureThread()
{
	// 初始化gdiplus
	GdiplusStartupInput in;
	ULONG_PTR gdiplus_token;
	GdiplusStartup(&gdiplus_token, &in, NULL);

	// 初始化手势
	GestureWindow wnd;
	wnd.Create(NULL, CRect(0, 0, 0, 0), L"GreenChromeGesture", WS_POPUP, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE);
	gesture_mgr.Init(&wnd);

	// 消息循环
	WTL::CMessageLoop msgLoop;
	int ret = msgLoop.Run();

	gesture_mgr.Exit();

	GdiplusShutdown(gdiplus_token);
}
