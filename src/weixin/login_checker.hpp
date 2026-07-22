#pragma once

#include "process.hpp"
#include "../app.hpp"
#include "utils.hpp"

namespace weixin {

template <typename Derived>
class LoginChecker : public Process<Derived> {
    using Base = Process<Derived>;

protected:
    ~LoginChecker() {
        if (timer_) {
            App::GetInstance().StopTimer(timer_);
        }
    }

    void CheckLogin() {
        timer_ = App::GetInstance().StartTimer(1000, [this, jump = false]() mutable -> bool {
            struct EnumData {
                DWORD pid;
                bool exited;  // 判断进程是否已经退出
                HWND hwnd;    // 查找到的主窗口句柄
                bool& jump;   // 是否已经模拟按键跳过登录确认框
            };
            EnumData data{this->GetPid(), true, nullptr, jump};
            EnumWindows(
                [](HWND hwnd, LPARAM lp) -> BOOL {
                    EnumData* pdata = reinterpret_cast<EnumData*>(lp);
                    DWORD pid = 0;
                    GetWindowThreadProcessId(hwnd, &pid);
                    if (pid != pdata->pid) {
                        return TRUE;
                    }
                    pdata->exited = false;

                    wchar_t name[256], title[256];
                    GetClassNameW(hwnd, name, sizeof(name));
                    if (!EndWith(name, L"QWindowIcon")) {
                        return TRUE;
                    }
                    GetWindowTextW(hwnd, title, sizeof(title));
                    if (title != std::wstring_view{L"微信"}) {
                        return TRUE;
                    }
                    pdata->hwnd = hwnd;
                    return FALSE;
                },
                reinterpret_cast<LPARAM>(&data));

            Debug("exited: %d, ", data.exited);
            Debug("hwnd: %p, ", static_cast<void*>(data.hwnd));
            Debug("jump: %d, ", data.jump);
            // 微信已经退出，结束 timer
            if (data.exited) {
                return false;
            }
            // 微信正在登录, 继续 timer 检测
            if (!data.hwnd) {
                return true;
            }
            if (!(GetWindowLongW(data.hwnd, GWL_STYLE) & WS_MAXIMIZEBOX)) {
                // 通过模拟按下enter键跳过登录确认窗口
                if (!jump && IsWindowVisible(data.hwnd)) {
                    jump = true;
                    // 新账号不会创建头像目录，也不需要模拟按键
                    if (std::filesystem::exists(GetHeadImgDir())) {
                        SendMessageW(data.hwnd, WM_KEYDOWN, VK_RETURN, 0);
                        SendMessageW(data.hwnd, WM_KEYUP, VK_RETURN, 0);
                    }
                }
                return true;
            }
            static_cast<Derived*>(this)->OnLogin(data.hwnd);
            return false;
        });
    }
    /** 微信登录成功时触发 */
    void OnLogin(HWND) {}

private:
    App::TimerHandler* timer_ = nullptr;
};

}  // namespace weixin