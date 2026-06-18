#include "login_win.hpp"

/** 单例进程检测 */
bool IsRunning() {
    HANDLE mutex = CreateMutexW(nullptr, FALSE, L"WeiXins_SingleInstance_Mutex");
    if (!mutex) {
        return true;
    }
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
        return false;
    }
    // 通知已经运行的进程
    for (int i = 0; i < 3; i++) {
        HWND hwnd = FindWindowW(CXXUI_WIN32_CLASS_NAME, L"");
        if (!hwnd) {
            Sleep(1000);
            continue;
        }
        COPYDATASTRUCT cds{};
        SendMessageW(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cds));
        break;
    }
    return true;
}

#define HOTKEY_WEIXIN 100  // 切换微信窗口
class MainWindow : public cxxui::Window<MainWindow> {
    using Base = cxxui::Window<MainWindow>;

public:
    MainWindow()
        : Base(cxxui::WindowOptions{}.SetParent(HWND_MESSAGE)) {
        RegisterHotKey(this->hwnd_, HOTKEY_WEIXIN, MOD_CONTROL | MOD_ALT, 'W');
    }
    ~MainWindow() override { UnregisterHotKey(this->hwnd_, HOTKEY_WEIXIN); }

protected:
    CXXUI_WIN_EVENT(MainWindow)
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_COPYDATA:
                LoginWindow::Open();
                break;
            case WM_HOTKEY:
                if (wp == HOTKEY_WEIXIN) {
                    WeiXinManager::GetInstance().Toggle();
                } else {
                    WeiXinManager::GetInstance().SetForeground(static_cast<int>(wp) - 101);
                }
                break;
            default:
                break;
        }
        return Base::OnWin32Msg(msg, wp, lp);
    }
};

int main() {
    if (IsRunning()) {
        return 1;
    }
    MainWindow win;
    WeiXinManager::GetInstance().Init(win.GetHandle());
    LoginWindow::Open();

    MSG msg{};
    while (true) {
        if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        } else if (WeiXinManager::GetInstance().IsAllExit() && LoginWindow::Count() == 0) {
            cxxui::Exit();
        }
    }
    return static_cast<int>(msg.wParam);
}
