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
                int count;   // 记录窗口句柄数量, 根据数量判断微信 正在登录、已退出还是登录成功
                bool& jump;  // 是否已经模拟按键跳过登录确认框
            };
            EnumData data{this->GetPid(), 0, jump};
            EnumWindows(
                [](HWND hwnd, LPARAM lp) -> BOOL {
                    EnumData* pdata = reinterpret_cast<EnumData*>(lp);
                    DWORD pid = 0;
                    GetWindowThreadProcessId(hwnd, &pid);
                    if (pid != pdata->pid) {
                        return TRUE;
                    }
                    pdata->count++;

#ifdef _DEBUG
                    char name[256];
                    fprintf(stderr, "hwnd: %p, ", hwnd);
                    GetWindowTextA(hwnd, name, sizeof(name));
                    fprintf(stderr, "title: %30s, ", name);
                    GetClassNameA(hwnd, name, sizeof(name));
                    fprintf(stderr, "class: %s\n", name);
#endif
                    return TRUE;
                },
                reinterpret_cast<LPARAM>(&data));
            // 微信已经退出，结束 timer
            if (data.count == 0) {
                return false;
            }
            // 确认微信主窗口已经创建
            HWND hwnd = weixin::GetMainWindow(data.pid);
            if (!hwnd) {
                return true;
            }
            // 微信正在登录, 继续 timer 检测
            if (data.count <= 6) {
                // 通过模拟按下enter键跳过登录确认窗口
                if (!jump) {
                    jump = true;
                    // 新账号不会创建头像目录，也不需要模拟按键
                    if (std::filesystem::exists(GetHeadImgDir())) {
                        SendMessageW(hwnd, WM_KEYDOWN, VK_RETURN, 0);
                        SendMessageW(hwnd, WM_KEYUP, VK_RETURN, 0);
                    }
                }
                return true;
            }
            // 解决可能获取到隐藏的登录确认框的问题
            auto style = GetWindowLongW(hwnd, GWL_STYLE);
            if (!(style & WS_MAXIMIZEBOX)) {
                return true;
            }
            static_cast<Derived*>(this)->OnLogin(hwnd);
            return false;
        });
    }
    /** 微信登录成功时触发 */
    void OnLogin(HWND) {}

private:
    App::TimerHandler* timer_ = nullptr;
};

}  // namespace weixin