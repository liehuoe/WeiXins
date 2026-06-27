#pragma once

#include <functional>
#include <cxxui/win.hpp>
#include "config.h"

class App : public cxxui::Window<App> {
    using Base = cxxui::Window<App>;

public:
    /**
     * @brief 互殴 app 的单实例对象
     *
     * @return App&
     */
    static App& GetInstance() noexcept {
        static App app;
        return app;
    }
    using RunningHandler = void (*)();
    /**
     * @brief 检查是否已经有其他的app进程正在运行
     *
     * @tparam Handler 其他进程的app调用IsRunning()会触发本进程的Handler
     * @return true 表示app已经启动
     */
    static bool IsRunning(RunningHandler handler) {
        HANDLE mutex = CreateMutexW(nullptr, FALSE, PROJECT_NAME L"_SingleInstance_Mutex");
        if (!mutex) {
            return true;
        }
        if (GetLastError() != ERROR_ALREADY_EXISTS) {
            running_handler_ = handler;
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
    /**
     * @brief 运行 app 的消息循环
     *
     * @tparam Handler
     * @param handler  执行用户自定义的消息处理函数
     * @return int 返回退出码
     */
    template <typename Handler>
    int Run(Handler&& handler = []() {}) {
        MSG msg{};
        while (true) {
            if (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    break;
                }
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
                continue;
            }
            handler();
        }
        return static_cast<int>(msg.wParam);
    }
    using TimerHandler = std::function<bool()>;
    /**
     * @brief 设置定时器
     *
     * @param ms 定时器的时间间隔，单位毫秒
     * @param handler 定时器的回调函数，回调函数返回false表示停止当前定时器，true则继续
     * @return TimerHandler* timer指针，用于停止定时器
     */
    TimerHandler* StartTimer(UINT ms, TimerHandler handler) {
        auto timer = new TimerHandler{std::move(handler)};
        // 先执行一次
        if (!(*timer)()) {
            KillTimer(GetHandle(), reinterpret_cast<UINT_PTR>(timer));
            delete timer;
            return nullptr;
        }
        SetTimer(GetHandle(),
                 reinterpret_cast<UINT_PTR>(timer),
                 ms,
                 [](HWND hwnd, UINT, UINT_PTR id_event, DWORD) {
                     TimerHandler* ptimer = reinterpret_cast<TimerHandler*>(id_event);
                     if (!(*ptimer)()) {
                         KillTimer(hwnd, id_event);
                         delete ptimer;
                     }
                 });
        return timer;
    }
    void StopTimer(TimerHandler* timer) {
        if (KillTimer(GetHandle(), reinterpret_cast<UINT_PTR>(timer))) {
            delete timer;
        }
    }
    using HotKeyHandler = std::function<void(int)>;
    /**
     * @brief 设置热键处理函数
     *
     * @param handler
     * @return bool 是否设置成功
     */
    bool SetHotKeyHandler(HotKeyHandler handler) noexcept {
        if (hotkey_handler_) {
            return false;
        }
        hotkey_handler_ = handler;
        return true;
    }
    /**
     * @brief 注册热键，参数参考RegisterHotKey
     *
     * @param id
     * @param modifiers
     * @param vk
     */
    void SetHotKey(int id, UINT modifiers, UINT vk) noexcept {
        RegisterHotKey(GetHandle(), id, modifiers, vk);
    }
    /**
     * @brief 注销热键
     *
     * @param id
     */
    void DelHotKey(int id) noexcept { UnregisterHotKey(GetHandle(), id); }

protected:
    CXXUI_WIN_EVENT(App)
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_COPYDATA:
                if (running_handler_) {
                    running_handler_();
                }
                break;
            case WM_HOTKEY:
                if (hotkey_handler_) {
                    hotkey_handler_(static_cast<int>(wp));
                }
                break;
            default:
                break;
        }
        return Base::OnWin32Msg(msg, wp, lp);
    }

private:
    App()
        : Base(cxxui::WindowOptions{}.SetParent(HWND_MESSAGE)) {}

    inline static RunningHandler running_handler_ = nullptr;
    inline static HotKeyHandler hotkey_handler_ = nullptr;
};
