#pragma once

#include <fstream>
#include <shlobj.h>
#include <cxxui/web_win.hpp>
#include <cxxui/web_win/js_msg_map.hpp>
#include "resource.h"
#include "utils.hpp"
#include "weixin_manager.hpp"
#include "weixin_runner.hpp"
#include "feature_win/dark_win.hpp"
#include "config.h"

template <typename Derived>
class WeiXinRunWindow : public DarkWindow<Derived> {
    using Base = DarkWindow<Derived>;
    struct TimerData {
        WeiXinRunWindow* win;
        DWORD wx_pid;           // 微信进程ID
        std::string login_dir;  // 登录的用户目录
    };

public:
    using Base::Base;
    /** 启动微信 */
    void RunWeiXin(std::string login_dir) {
        WeiXinRunner wx;
        auto pd = wx.Run();
        // 检查微信登录状态
        TimerData* timer = new TimerData{this, pd->GetPid(), login_dir};  // NEW: new TimerData
        SetTimer(this->hwnd_, reinterpret_cast<UINT_PTR>(timer), 1000, OnCheckTimer);
        // 检测和管理微信子进程
        WeiXinManager::GetInstance().WaitForExit(std::move(login_dir), std::move(pd));
    }

private:
    static void OnCheckTimer(HWND, UINT, UINT_PTR id_event, DWORD) {
        TimerData* timer = reinterpret_cast<TimerData*>(id_event);
        struct EnumData {
            DWORD pid;
            // 记录窗口句柄数量, 根据数量判断微信 正在登录、已退出还是登录成功
            int count;
        };
        EnumData data{timer->wx_pid, 0};
        EnumWindows(
            [](HWND hwnd, LPARAM lp) -> BOOL {
                EnumData* data = reinterpret_cast<EnumData*>(lp);
                DWORD pid = 0;
                GetWindowThreadProcessId(hwnd, &pid);
                if (pid != data->pid) {
                    return TRUE;
                }
                data->count++;

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
        if (data.count > 0 && data.count <= 6) {
            return;  // 微信正在登录, 继续 timer 检测
        }

        if (data.count > 6) {  // 登录成功
            if (!WeiXinManager::GetInstance().EmitLogin(timer->wx_pid)) {
                return;
            }
            static_cast<Derived*>(timer->win)->OnLogin(timer->login_dir, false);
        } else {  // 微信退出
            static_cast<Derived*>(timer->win)->OnLogin(timer->login_dir, true);
        }
        // 清理资源
        KillTimer(timer->win->hwnd_, id_event);
        delete timer;  // DEL: new TimerData
    }
};

class LoginWindow : public WeiXinRunWindow<LoginWindow> {
    using Base = WeiXinRunWindow<LoginWindow>;

private:
    LoginWindow()
        : Base(cxxui::WindowOptions().SetTitle("微信多开助手").SetWidth(300).SetHeight(400)) {
        SetIcon(IDI_LOGO);
    }
    CXXUI_WIN_EVENT(LoginWindow)
    CXXUI_WEB_EVENT(LoginWindow)
    void OnWebCreated(std::optional<cxxui::WindowError> err) {
        Base::OnWebCreated(err);
        InitJsMsg();
        SetUrl(PAGE(login));
        SetForeground(this->hwnd_);
    }
    void OnClosed() {
        if (win_.get() == this) {
            win_ = nullptr;
        }
        Base::OnClosed();
    }
    /** 处理登录窗口下发的请求 */
private:
    cxxui::JsMsgMap<> map_;
    void InitJsMsg() {
        map_.bind("/app/version", OnAppVersion);
        map_.bind("/user/get", OnUserGet);
        map_.bind("/user/login", [this](cxxui::json& arg) { return OnUserLogin(arg); });
        map_.bind("/user/set", OnUserSet);
        OnJsMsg(map_.GetHandler());
    }
    static cxxui::json OnAppVersion(cxxui::json&) { return PROJECT_VERSION; }
    static cxxui::json OnUserGet(cxxui::json&) {
        return cxxui::json::parse(std::ifstream{Config::GetInstance().GetCfgPath()});
    }
    cxxui::json OnUserLogin(cxxui::json& arg) {
        namespace fs = std::filesystem;
        // 拷贝用户配置
        std::string login_dir = arg.at("dir").get<std::string>();
        UpdateConfig(login_dir);
        // 删除头像目录
        fs::path head_imgs = Config::GetInstance().GetHeadImgDir();
        if (fs::exists(head_imgs)) {
            fs::remove_all(head_imgs);
        }
        // 先隐藏, 等待微信登录后或退出后再关闭窗口
        Show(false);
        if (win_.get() == this) {
            [[maybe_unused]] auto ptr = win_.release();  // NEW: win_.release
        }
        RunWeiXin(std::move(login_dir));
        return nullptr;
    }
    friend class WeiXinRunWindow<LoginWindow>;
    void OnLogin(const std::string& login_dir, bool exited) {
        if (!exited) {
            UpdateHeadImg(login_dir);        // 更新头像
            UpdateConfig<false>(login_dir);  // 更新配置文件
        }
        delete this;  // DEL: win_.release
    }
    static cxxui::json OnUserSet(cxxui::json& arg) {
        namespace fs = std::filesystem;
        cxxui::json users;
        const fs::path& app_dir = Config::GetInstance().GetUserDir();
        for (const auto& user : arg) {
            if (user.contains("name") && !user.at("name").empty()) {
                users.emplace_back(user);
                continue;
            }
            // 删除账号目录
            fs::path user_dir = app_dir / user.at("dir").get<std::string>();
            if (fs::exists(user_dir)) {
                fs::remove_all(user_dir);
            }
        }
        if (!fs::exists(app_dir)) {
            fs::create_directories(app_dir);
        }
        std::ofstream{Config::GetInstance().GetCfgPath()} << users.dump(4);
        return nullptr;
    }
    /** 更新配置文件到微信/程序 */
    template <bool to_weixin = true>
    void UpdateConfig(const std::string& login_dir) {
        namespace fs = std::filesystem;
        fs::path cfg_dir = Config::GetInstance().GetUserDir() / login_dir;
        if (!fs::exists(cfg_dir)) {
            fs::create_directories(cfg_dir);
        }
        constexpr std::string_view kFileList[] = {"global_config", "global_config.crc"};
        fs::path src_dir, dst_dir;
        if constexpr (to_weixin) {
            src_dir = cfg_dir;
            dst_dir = Config::GetInstance().GetWeiXinDir() / "config";
        } else {
            src_dir = Config::GetInstance().GetWeiXinDir() / "config";
            dst_dir = cfg_dir;
        }
        for (const auto file : kFileList) {
            fs::path src = src_dir / file;
            fs::path dst = dst_dir / file;
            if (fs::exists(dst)) {
                fs::remove(dst);
            }
            if (fs::exists(src)) {
                CopyFileExW(src.c_str(), dst.c_str(), nullptr, nullptr, nullptr, 0);
            }
        }
    }
    /** 更新微信头像到程序 */
    void UpdateHeadImg(const std::string& login_dir) {
        namespace fs = std::filesystem;
        auto GetFirstFile = [](const fs::path& dir, std::string_view exclude = {}) {
            for (const auto& entry : fs::directory_iterator(dir)) {
                if (!exclude.empty() && entry.path().filename() == exclude) {
                    continue;
                }
                return entry.path();
            }
            return fs::path{};
        };
        // 更新微信头像
        fs::path logo_dir = GetFirstFile(Config::GetInstance().GetHeadImgDir(), "0");
        if (!logo_dir.empty()) {
            fs::path logo_src = GetFirstFile(logo_dir);
            if (!logo_src.empty()) {
                fs::path logo_dst = Config::GetInstance().GetUserDir() / login_dir / "logo.jpg";
                CopyFileExW(logo_src.c_str(), logo_dst.c_str(), nullptr, nullptr, nullptr, 0);
            }
        }
    }
    /** 管理登录窗口，只打开一个登录窗口 */
public:
    static void Open() {
        if (win_) {
            SetForeground(win_->hwnd_);
        } else {
            win_ = std::unique_ptr<LoginWindow>{new LoginWindow{}};
        }
    }

private:
    inline static std::unique_ptr<LoginWindow> win_;
};
