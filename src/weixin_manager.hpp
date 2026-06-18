#pragma once

#include <vector>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>
#include "utils.hpp"
#include "config.hpp"
#include "weixin_runner.hpp"
#include <gdiplus.h>

template <typename Derived>
class Singleton {
protected:
    static Derived& GetInstance() {
        static Derived instance;
        return instance;
    }
};

/** 管理主进程 */
template <typename Derived>
class MainManager : public Singleton<Derived> {
public:
    void Init(HWND hwnd) { hwnd_ = hwnd; }
    /**
     * @brief 设置快捷键
     */
    template <bool is_register>
    void SetHotKey(int idx) {
        if constexpr (is_register) {
            RegisterHotKey(hwnd_, 100 + idx, MOD_CONTROL | MOD_SHIFT, '0' + idx);
        } else {
            UnregisterHotKey(hwnd_, 100 + idx);
        }
    }

private:
    HWND hwnd_ = nullptr;
};

/** 检测微信进程是否退出 */
template <typename Derived>
class WeiXinChecker : public MainManager<Derived> {
public:
    /**
     * @brief 检测微信进程是否退出
     *
     * @return 所有微信进程退出返回true, 否则返回false
     */
    bool IsAllExit() {
        while (true) {
            if (handles_.empty()) {
                return true;
            }
            // 判断微信进程是否退出
            DWORD rc = MsgWaitForMultipleObjects(
                static_cast<DWORD>(handles_.size()), handles_.data(), FALSE, INFINITE, QS_ALLINPUT);
            if (rc < WAIT_OBJECT_0 || rc >= WAIT_OBJECT_0 + handles_.size()) {
                return false;
            }
            Remove(rc - WAIT_OBJECT_0);
        }
        return false;
    }
    /**
     * @brief 添加需要等待退出的微信进程
     */
    void WaitForExit(std::string login_dir, std::unique_ptr<ProcessData> pd) {
        static_cast<Derived*>(this)->OnCreated(static_cast<int>(handles_.size()), pd);
        handles_.emplace_back(pd->GetProcess());
        dirs_.emplace_back(std::move(login_dir));
        pds_.emplace_back(std::move(pd));
    }

protected:
    const std::vector<std::string>& GetDirs() const noexcept { return dirs_; }
    const std::vector<std::unique_ptr<ProcessData>>& GetPds() const noexcept { return pds_; }
    void OnCreated(int, const std::unique_ptr<ProcessData>&) {}
    bool OnLogin(int) { return true; }
    void OnClosed(int, const std::unique_ptr<ProcessData>&) {}

private:
    std::vector<HANDLE> handles_;
    std::vector<std::string> dirs_;
    std::vector<std::unique_ptr<ProcessData>> pds_;
    void Remove(int idx) {
        handles_.erase(handles_.begin() + idx);
        auto& pd = pds_[idx];
        static_cast<Derived*>(this)->OnClosed(idx, pd);
        pds_.erase(pds_.begin() + idx);
        dirs_.erase(dirs_.begin() + idx);
    }
};

template <typename Derived>
class WeiXinSwitcher : public WeiXinChecker<Derived> {
    using Base = WeiXinChecker<Derived>;

public:
    bool EmitLogin(DWORD pid) {
        const std::vector<std::unique_ptr<ProcessData>>& pds = this->GetPds();
        auto rit =
            std::find_if(pds.rbegin(), pds.rend(), [pid](const std::unique_ptr<ProcessData>& pd) {
                return pd->GetPid() == pid;
            });
        auto idx = static_cast<int>(std::prev(rit.base()) - pds.begin());
        return static_cast<Derived*>(this)->OnLogin(idx);
    }
    /** 切换微信窗口 */
    void Toggle() {
        if (focus_index_ >= static_cast<int>(mains_.size())) {
            focus_index_ = 0;
        }
        SetForeground(focus_index_++);
    }
    /** 设置微信窗口到前台 */
    void SetForeground(int idx) {
        if (mains_.empty()) {
            return;
        }
        if (idx >= static_cast<int>(mains_.size())) {
            idx = 0;
        }
        HWND hwnd = mains_[idx];
        if (IsWindowVisible(hwnd)) {
            ::SetForeground(hwnd);
        } else {
            TrayShow(idx);
        }
    }

protected:
    friend class WeiXinChecker<Derived>;
    void OnCreated(int idx, const std::unique_ptr<ProcessData>& pd) {
        mains_.emplace_back(nullptr);
        trays_.emplace_back(nullptr);
        this->template SetHotKey<true>(static_cast<int>(mains_.size()));
        constexpr DWORD flags = WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS;
        auto hook = SetWinEventHook(
            EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, nullptr, OnHook, pd->GetPid(), 0, flags);
        hooks_.emplace_back(hook);
        Base::OnCreated(idx, pd);
    }
    bool OnLogin(int idx) {
        HWND main, tray;
        GetWeiXinWindow(this->GetPds()[idx]->GetPid(), main, tray);
        if (!main) {
            return false;
        }
        mains_[idx] = main;
        main_idx_map_.emplace(main, idx);
        trays_[idx] = tray;
        focus_index_ = 0;
        return Base::OnLogin(idx);
    }
    void OnClosed(int idx, const std::unique_ptr<ProcessData>& pd) {
        this->template SetHotKey<false>(static_cast<int>(mains_.size()));
        mains_.erase(mains_.begin() + idx);
        for (auto it = main_idx_map_.begin(); it != main_idx_map_.end();) {
            if (it->second == idx) {
                it = main_idx_map_.erase(it);
            } else {
                if (it->second > idx) {
                    --it->second;
                }
                ++it;
            }
        }
        trays_.erase(trays_.begin() + idx);
        hooks_.erase(hooks_.begin() + idx);
        if (focus_index_ > idx) {
            --focus_index_;
        }
        Base::OnClosed(idx, pd);
    }
    const std::vector<HWND>& GetMains() const noexcept { return mains_; }

private:
    int focus_index_ = 0;
    std::vector<HWND> mains_;
    std::map<HWND, int> main_idx_map_;
    std::vector<HWND> trays_;
    struct Hook {
        Hook(HWINEVENTHOOK h) noexcept
            : hook(h) {}
        Hook(Hook&& other) noexcept {
            hook = other.hook;
            other.hook = nullptr;
        }
        ~Hook() {
            if (hook) {
                UnhookWinEvent(hook);
            }
        }
        Hook& operator=(Hook&& other) noexcept {
            if (this != &other) {
                hook = other.hook;
                other.hook = nullptr;
            }
            return *this;
        }
        HWINEVENTHOOK hook;
    };
    std::vector<Hook> hooks_;
    void TrayShow(int idx) {
        if (trays_.empty()) {
            return;
        }
        if (idx < 0 || idx >= static_cast<int>(trays_.size())) {
            idx = 0;
        }
        SendMessageW(trays_[idx], WM_APP + 901, 0, 0x0400);
    }
    static void OnHook(
        HWINEVENTHOOK, DWORD event, HWND hwnd, LONG id_object, LONG id_child, DWORD, DWORD) {
        if (id_object != OBJID_WINDOW || id_child != CHILDID_SELF) {
            return;
        }
        WeiXinSwitcher<Derived>& self = Singleton<Derived>::GetInstance();
        auto it = self.main_idx_map_.find(hwnd);
        if (it == self.main_idx_map_.end()) {
            return;
        }
        switch (event) {
            case EVENT_OBJECT_SHOW:
                self.focus_index_ = it->second + 1;
                break;
            case EVENT_OBJECT_HIDE:
                self.focus_index_ = it->second;
                break;
            default:
                break;
        }
    }
    static void GetWeiXinWindow(DWORD pid, HWND& main, HWND& tray) {
        struct EnumData {
            DWORD pid;
            HWND main;
            HWND tray;
        };
        EnumData data{pid, nullptr, nullptr};
        EnumWindows(
            [](HWND hwnd, LPARAM lp) -> BOOL {
                EnumData* pdata = reinterpret_cast<EnumData*>(lp);
                DWORD cur_pid = 0;
                GetWindowThreadProcessId(hwnd, &cur_pid);
                if (cur_pid != pdata->pid) {
                    return TRUE;
                }
                char name[256];
                GetClassNameA(hwnd, name, sizeof(name));
                std::string_view str{name};
                if (EndWith(str, "QWindowIcon")) {
                    if (IsWindowVisible(hwnd) && GetRenderWindow(hwnd) != nullptr) {
                        pdata->main = hwnd;
                    }
                } else if (EndWith(str, "WxTrayIconMessageWindowClass")) {
                    pdata->tray = hwnd;
                }
                if (pdata->main && pdata->tray) {
                    return FALSE;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&data));
        main = data.main;
        tray = data.tray;
    }
    static HWND GetRenderWindow(HWND hwnd) {
        HWND render_hwnd = nullptr;
        EnumChildWindows(
            hwnd,
            [](HWND h, LPARAM lp) {
                HWND* prender_hwnd = reinterpret_cast<HWND*>(lp);
                char name[256];
                GetClassNameA(h, name, 256);
                if (std::string_view{name} == "MMUIRenderSubWindowHW") {
                    *prender_hwnd = h;
                    return FALSE;
                } else {
                    return TRUE;
                }
            },
            reinterpret_cast<LPARAM>(&render_hwnd));
        return render_hwnd;
    }
};

template <typename Derived>
class WeiXinSysMenu : public WeiXinSwitcher<Derived> {
    using Base = WeiXinSwitcher<Derived>;

public:
    WeiXinSysMenu()
        : Base() {
        Gdiplus::GdiplusStartupInput startupInput;
        startupInput.GdiplusVersion = 1;
        Gdiplus::GdiplusStartup(&gdi_token_, &startupInput, nullptr);
    };
    ~WeiXinSysMenu() { Gdiplus::GdiplusShutdown(gdi_token_); };

protected:
    friend class WeiXinChecker<Derived>;
    void OnCreated(int idx, const std::unique_ptr<ProcessData>& pd) {
        icons_.emplace_back(nullptr);
        Base::OnCreated(idx, pd);
    }
    bool OnLogin(int idx) {
        if (!Base::OnLogin(idx)) {
            return false;
        }
        HWND hwnd = this->GetMains()[idx];
        UpdateTitle(idx, hwnd);
        UpdateIcon(idx, hwnd);
        return true;
    }
    void OnClosed(int idx, const std::unique_ptr<ProcessData>& pd) {
        icons_.erase(icons_.begin() + idx);
        Base::OnClosed(idx, pd);
    }

private:
    ULONG_PTR gdi_token_;
    struct Icon {
        HICON value;
        Icon(HICON val) noexcept
            : value(val) {}
        Icon(Icon&& other) noexcept {
            value = other.value;
            other.value = nullptr;
        }
        ~Icon() {
            if (value) {
                DestroyIcon(value);
            }
        }
        Icon& operator=(Icon&& other) noexcept {
            if (this != &other) {
                value = other.value;
                other.value = nullptr;
            }
            return *this;
        }
    };
    std::vector<Icon> icons_;

    /** 更新微信任务栏的标题 */
    void UpdateTitle(int idx, HWND hwnd) {
        auto root = nlohmann::json::parse(std::ifstream{Config::GetInstance().GetCfgPath()});
        if (!root.is_array()) {
            return;
        }
        std::string login_name;
        const std::string& login_dir = this->GetDirs()[idx];
        for (const auto& item : root) {
            std::string name = item.value("name", "");
            std::string dir = item.value("dir", "");
            if (name.empty() || dir.empty()) {
                continue;
            }
            if (dir != login_dir) {
                continue;
            }
            login_name = std::move(name);
            break;
        }
        if (login_name.empty()) {
            return;
        }
        login_name = std::to_string(idx + 1) + ". " + login_name;
        SetWindowTextW(hwnd, cxxui::detail::U82W(login_name).c_str());
    }
    /** 更新微信任务栏的图标 */
    void UpdateIcon(int idx, HWND hwnd) {
        // 加载图片
        auto jpg_path = Config::GetInstance().GetUserDir() / this->GetDirs()[idx] / kHeadImgName;
        using namespace Gdiplus;
        struct Jpg {
            Bitmap* value;
            Jpg(Bitmap* val) noexcept
                : value(val) {}
            ~Jpg() {
                if (value) {
                    delete value;
                }
            }
        };
        Jpg jpg = Bitmap::FromFile(jpg_path.c_str());
        if (!jpg.value || jpg.value->GetLastStatus() != Ok) {
            return;
        }
        // 裁剪图片
        constexpr int size = 256;
        constexpr int radius = size / 2;
        Jpg resized = new Bitmap(size, size, PixelFormat32bppARGB);
        Graphics graphics(resized.value);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        // 圆角
        graphics.Clear(Color(0, 0, 0, 0));
        GraphicsPath path;
        path.AddArc(0, 0, radius, radius, 180, 90);
        path.AddArc(0 + size - radius, 0, radius, radius, 270, 90);
        path.AddArc(0 + size - radius, 0 + size - radius, radius, radius, 0, 90);
        path.AddArc(0, 0 + size - radius, radius, radius, 90, 90);
        path.CloseFigure();
        graphics.SetClip(&path, CombineModeReplace);

        graphics.DrawImage(jpg.value, 0, 0, size, size);
        // 获取 HICON
        HICON icon;
        if (resized.value->GetHICON(&icon) != Ok) {
            return;
        }
        icons_[idx] = icon;
        LPARAM lp = reinterpret_cast<LPARAM>(icon);
        SendMessageW(hwnd, WM_SETICON, ICON_BIG, lp);
        SendMessageW(hwnd, WM_SETICON, ICON_SMALL, lp);
    }
};

class WeiXinManager : public WeiXinSysMenu<WeiXinManager> {
    using Base = WeiXinSysMenu<WeiXinManager>;

public:
    static WeiXinManager& GetInstance() { return Singleton<WeiXinManager>::GetInstance(); }

protected:
    friend class WeiXinSwitcher<WeiXinManager>;  // for OnHook
    friend class WeiXinChecker<WeiXinManager>;
    void OnCreated(int idx, const std::unique_ptr<ProcessData>& pd) { Base::OnCreated(idx, pd); }
    void OnClosed(int idx, const std::unique_ptr<ProcessData>& pd) { Base::OnClosed(idx, pd); }

private:
    friend class Singleton<WeiXinManager>;
    WeiXinManager() = default;
};
