// 微信启动：不触发任何事件，没有焦点
#pragma once

#include <vector>
#include <map>
#include "utils.hpp"
#include "weixin_runner.hpp"

/** 检测微信进程是否退出 */
template <typename Derived>
class WeiXinChecker {
public:
    bool Run() {
        while (true) {
            if (handles_.empty()) {
                return false;
            }
            // 判断微信进程是否退出
            DWORD rc = MsgWaitForMultipleObjects(
                static_cast<DWORD>(handles_.size()), handles_.data(), FALSE, INFINITE, QS_ALLINPUT);
            if (rc < WAIT_OBJECT_0 || rc >= WAIT_OBJECT_0 + handles_.size()) {
                return true;
            }
            Remove(rc - WAIT_OBJECT_0);
        }
        return true;
    }
    void WaitForExit(std::string login_dir, std::unique_ptr<ProcessData> pd) {
        static_cast<Derived*>(this)->OnCreated(static_cast<int>(handles_.size()), pd);
        handles_.emplace_back(pd->GetProcess());
        login_dirs.emplace_back(std::move(login_dir));
        pds_.emplace_back(std::move(pd));
    }

protected:
    std::vector<HANDLE> handles_;
    std::vector<std::string> login_dirs;
    std::vector<std::unique_ptr<ProcessData>> pds_;

    void OnCreated(int, const std::unique_ptr<ProcessData>&) {}
    void OnClosed(int, const std::unique_ptr<ProcessData>&) {}

private:
    void Remove(int idx) {
        handles_.erase(handles_.begin() + idx);
        login_dirs.erase(login_dirs.begin() + idx);
        auto& pd = pds_.at(idx);
        static_cast<Derived*>(this)->OnClosed(idx, pd);
        pds_.erase(pds_.begin() + idx);
    }
};

template <typename Derived>
class WeiXinTray : public WeiXinChecker<Derived> {
    using Base = WeiXinChecker<Derived>;

public:
    bool EmitLogin(DWORD pid) {
        auto rit =
            std::find_if(pds_.rbegin(), pds_.rend(), [pid](const std::unique_ptr<ProcessData>& pd) {
                return pd->GetPid() == pid;
            });
        if (rit == pds_.rend()) {
            return true;
        }
        auto idx = static_cast<int>(std::prev(rit.base()) - pds_.begin());
        return static_cast<Derived*>(this)->OnLogin(idx, *rit);
    }
    /** 切换微信窗口 */
    void Switch() {
        static int sw_idx = -1;
        if (++sw_idx >= static_cast<int>(trays_.size())) {
            sw_idx = 0;
        }
        TrayShow(sw_idx);
    }

protected:
    std::vector<HWND> trays_;
    void TrayShow(int idx) { SendMessageW(trays_.at(idx), WM_APP + 901, 0, 0x0400); }

    friend class WeiXinChecker<Derived>;
    void OnCreated(int idx, const std::unique_ptr<ProcessData>& pd) {
        trays_.emplace_back(nullptr);
        Base::OnCreated(idx, pd);
    }
    bool OnLogin(int idx, const std::unique_ptr<ProcessData>& pd) {
        trays_[idx] = GetTrayWindow(pd->GetPid());
        return true;
    }
    void OnClosed(int idx, const std::unique_ptr<ProcessData>& pd) {
        trays_.erase(trays_.begin() + idx);
        Base::OnCreated(idx, pd);
    }

private:
    static HWND GetTrayWindow(DWORD pid) {
        struct EnumData {
            DWORD pid;
            HWND hwnd;
        };
        EnumData data{pid, nullptr};
        EnumWindows(
            [](HWND hwnd, LPARAM lp) -> BOOL {
                EnumData* data = reinterpret_cast<EnumData*>(lp);
                DWORD pid = 0;
                GetWindowThreadProcessId(hwnd, &pid);
                if (pid != data->pid) {
                    return TRUE;
                }
                char name[256];
                GetClassNameA(hwnd, name, sizeof(name));
                std::string_view str{name};
                if (EndWith(str, "WxTrayIconMessageWindowClass")) {
                    data->hwnd = hwnd;
                    return FALSE;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&data));
        return data.hwnd;
    }
};

class WeiXinManager : public WeiXinTray<WeiXinManager> {
    using Base = WeiXinTray<WeiXinManager>;

public:
    static WeiXinManager& GetInstance() {
        static WeiXinManager instance;
        return instance;
    }

protected:
    friend class WeiXinChecker<WeiXinManager>;
    void OnCreated(int idx, const std::unique_ptr<ProcessData>& pd) { Base::OnCreated(idx, pd); }
    void OnClosed(int idx, const std::unique_ptr<ProcessData>& pd) { Base::OnCreated(idx, pd); }

private:
    WeiXinManager() = default;
};
