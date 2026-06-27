#pragma once

#include "login_checker.hpp"

namespace weixin {

struct Hook {
    HWINEVENTHOOK value;
    ~Hook() {
        if (value) {
            UnhookWinEvent(value);
        }
    }
};

template <typename Derived>
class HotKey : public LoginChecker<Derived> {
    using Base = LoginChecker<Derived>;

protected:
    HotKey() {
        if (App::GetInstance().SetHotKeyHandler(OnHotKey)) {
            App::GetInstance().SetHotKey(0, MOD_CONTROL | MOD_ALT, 'W');
        }
    };
    ~HotKey() { hwnd_map_.erase(hwnd_); }

public:
    /**
     * @brief 启动微信
     * @return bool 是否启动成功
     */
    bool Start() noexcept {
        if (!Base::Start()) {
            return false;
        }
        int size = static_cast<int>(Base::GetAllObjs().size());
        App::GetInstance().SetHotKey(size, MOD_CONTROL | MOD_SHIFT, '0' + size);
        return true;
    }
    /**
     * @brief 获取微信主窗口句柄
     *
     * @return HWND
     */
    HWND GetMainHwnd() const noexcept { return hwnd_; }

protected:
    friend class CloseChecker<Derived>;
    /** 微信退出时触发 */
    void OnClose(HANDLE handle) {
        App::GetInstance().DelHotKey(static_cast<int>(Base::GetAllObjs().size()));

        Base::OnClose(handle);
    }
    friend class LoginChecker<Derived>;
    /** 微信登录成功时触发 */
    void OnLogin(HWND hwnd) {
        hwnd_ = hwnd;
        tray_ = weixin::GetTrayWindow(this->GetPid());
        constexpr DWORD flags = WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS;
        hook_.value = SetWinEventHook(
            EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, nullptr, OnHook, this->GetPid(), 0, flags);
        hwnd_map_[hwnd] = static_cast<Derived*>(this);

        Base::OnLogin(hwnd);
    }

private:
    HWND hwnd_ = nullptr;  // 微信主窗口
    HWND tray_ = nullptr;  // 微信托盘图标窗口
    Hook hook_;
    inline static std::unordered_map<HWND, Derived*> hwnd_map_;
    inline static int focus_index_ = 0;

    static void OnHotKey(int id) {
        if (id == 0) {
            Toggle();
        } else {
            auto self = static_cast<HotKey*>(Base::GetAllObjs()[id - 1]);
            if (self) {
                self->SetForeground();
            }
        }
    }
    static void OnHook(
        HWINEVENTHOOK, DWORD event, HWND hwnd, LONG id_object, LONG id_child, DWORD, DWORD) {
        if (id_object != OBJID_WINDOW || id_child != CHILDID_SELF) {
            return;
        }

        auto it = hwnd_map_.find(hwnd);
        if (it == hwnd_map_.end()) {
            return;
        }
        switch (event) {
            case EVENT_OBJECT_SHOW:
                focus_index_ = it->second->GetIndex() + 1;  // 指向下一个微信
                break;
            case EVENT_OBJECT_HIDE:
                focus_index_ = it->second->GetIndex();  // 指向该隐藏的微信
                break;
            default:
                break;
        }
    }
    /** 设置微信窗口到前台 */
    void SetForeground() {
        if (IsWindowVisible(hwnd_)) {
            ::SetForeground(hwnd_);
        } else {
            SendMessageW(tray_, WM_APP + 901, 0, 0x0400);
        }
    }
    /** 切换微信窗口 */
    static void Toggle() {
        auto& objs = Base::GetAllObjs();
        if (objs.empty()) {
            return;
        }
        if (focus_index_ >= static_cast<int>(objs.size())) {
            focus_index_ = 0;
        }
        objs[focus_index_]->SetForeground();
        focus_index_++;
    }
};

}  // namespace weixin