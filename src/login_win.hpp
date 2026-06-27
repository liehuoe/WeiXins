#pragma once

#include <fstream>
#include <shlobj.h>
#include <cxxui/web_win/js_msg_map.hpp>
#include "resource.h"
#include "feature_win/dark_win.hpp"
#include "config.h"
#include "weixin.hpp"

class LoginWindow : public DarkWindow<LoginWindow> {
    using Base = DarkWindow<LoginWindow>;

private:
    LoginWindow()
        : Base(cxxui::WindowOptions{}.SetTitle("微信多开助手").SetWidth(300).SetHeight(400)) {
        SetIcon(IDI_LOGO);
    }
    CXXUI_WIN_EVENT(LoginWindow)
    void OnClosed() {
        win_ = nullptr;
        Base::OnClosed();
    }
    CXXUI_WEB_EVENT(LoginWindow)
    void OnWebCreated(std::optional<cxxui::WindowError> err) {
        Base::OnWebCreated(err);
        InitJsMsg();
        SetUrl(PAGE(login));
        SetForeground(this->hwnd_);
    }
    /** 处理登录窗口下发的请求 */
private:
    cxxui::JsMsgMap<> map_;
    void InitJsMsg() {
        map_.bind("/app/version", OnAppVersion);
        map_.bind("/user/get", OnUserGet);
        map_.bind("/user/getDirs", OnUserGetDirs);
        map_.bind("/user/login", [this](cxxui::json& arg) { return OnUserLogin(arg); });
        map_.bind("/user/set", OnUserSet);
        OnJsMsg(map_.GetHandler());
    }
    static cxxui::json OnAppVersion(cxxui::json&) { return PROJECT_VERSION; }
    static cxxui::json OnUserGet(cxxui::json&) {
        return cxxui::json::parse(std::ifstream{Config::GetInstance().GetCfgPath()});
    }
    static cxxui::json OnUserGetDirs(cxxui::json&) {
        cxxui::json dirs;
        for (const auto& obj : WeiXin::GetAllObjs()) {
            dirs.emplace_back(obj->GetDir());
        }
        return dirs;
    }
    cxxui::json OnUserLogin(cxxui::json& arg) {
        Show(false);  // 避免闪烁
        WeiXin::Create(arg.value("dir", ""));
        win_ = nullptr;
        return nullptr;
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
    /** 管理登录窗口，只打开一个登录窗口 */
public:
    static void Open() {
        if (win_) {
            SetForeground(win_->GetHandle());
        } else {
            win_ = std::unique_ptr<LoginWindow>{new LoginWindow{}};
        }
    }
    static bool IsOpen() noexcept { return win_ != nullptr; }

private:
    inline static std::unique_ptr<LoginWindow> win_;
};
