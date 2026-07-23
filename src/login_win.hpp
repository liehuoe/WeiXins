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

public:
    ~LoginWindow() { SaveData(true); }

private:
    LoginWindow()
        : Base(cxxui::WindowOptions{}.SetTitle("微信多开助手").SetWidth(300).SetHeight(400)) {
        SetIcon(IDI_LOGO);
        WeiXin::SetCloseHandler(Refreseh);
    }
    CXXUI_WIN_EVENT(LoginWindow)
    void OnClosed() {
        Base::OnClosed();
        win_ = nullptr;
    }
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_SIZE: {
                if (!this->ctrl_) {
                    break;
                }
                if (wp == SIZE_MAXIMIZED) {
                    RunJs("window.onMaximize(true)");
                } else if (wp == SIZE_RESTORED) {
                    RunJs("window.onMaximize(false)");
                }
                break;
            }
            default:
                break;
        }
        return Base::OnWin32Msg(msg, wp, lp);
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
    /*
    {
        user: User[];       //用户数据
        isMulti?: Boolean;  // 是否处于多选模式
        selected?: number[] // 已勾选的下标数组
    }
    */
    cxxui::json data_;
    bool data_modified_ = false;
    void LoadData() {
        if (data_modified_) {
            return;
        }
        auto data = cxxui::json::parse(std::ifstream{Config::GetInstance().GetCfgPath()});
        if (data.is_array()) {
            data_["user"] = data;
        } else if (data.is_object()) {
            data_ = data;
        } else {
            data_["user"] = cxxui::json::array();
        }
    }
    void SaveData(bool force = false) {
        if (!force) {
            data_modified_ = true;
            return;
        }
        if (!data_modified_) {
            return;
        }
        data_modified_ = false;
        namespace fs = std::filesystem;
        const fs::path& app_dir = Config::GetInstance().GetUserDir();
        if (!fs::exists(app_dir)) {
            fs::create_directories(app_dir);
        }
        std::ofstream{Config::GetInstance().GetCfgPath()} << data_.dump(4);
    }
    void InitJsMsg() {
        map_.bind("/app/version", OnAppVersion);
        map_.bind("/app/close", [this](cxxui::json& arg) { return OnAppClose(arg); });
        map_.bind("/app/maximize", [this](cxxui::json& arg) { return OnAppMaximize(arg); });
        map_.bind("/app/minimize", [this](cxxui::json& arg) { return OnAppMinimize(arg); });
        map_.bind("/app/drag", [this](cxxui::json& arg) { return OnAppDrag(arg); });
        map_.bind("/app/pin", [this](cxxui::json& arg) { return OnAppPin(arg); });
        map_.bind("/user/get", [this](cxxui::json& arg) { return OnUserGet(arg); });
        map_.bind("/user/getDirs", OnUserGetDirs);
        map_.bind("/user/login", [this](cxxui::json& arg) { return OnUserLogin(arg); });
        map_.bind("/user/set", [this](cxxui::json& arg) { return OnUserSet(arg); });
        map_.bind("/user/setMulti", [this](cxxui::json& arg) { return OnUserSetMulti(arg); });
        map_.bind("/user/move", [this](cxxui::json& arg) { return OnUserMove(arg); });
        OnJsMsg(map_.GetHandler());
    }
    /*
    获取版本号
    arg: null
    ret: string 版本号
    */
    static cxxui::json OnAppVersion(cxxui::json&) { return PROJECT_VERSION; }
    cxxui::json OnAppClose(cxxui::json&) {
        Close();
        return nullptr;
    }
    cxxui::json OnAppMaximize(cxxui::json&) {
        bool is_max = IsZoomed(this->hwnd_);
        if (is_max) {
            ShowWindow(this->hwnd_, SW_RESTORE);
        } else {
            ShowWindow(this->hwnd_, SW_MAXIMIZE);
        }
        return nullptr;
    }
    cxxui::json OnAppMinimize(cxxui::json&) {
        ShowWindow(this->hwnd_, SW_MINIMIZE);
        return nullptr;
    }
    cxxui::json OnAppDrag(cxxui::json&) {
        ReleaseCapture();
        PostMessageW(GetHandle(), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, 0);
        return nullptr;
    }
    /*
    置顶窗口
    arg: { pin?: boolean }
    ret: boolean 置顶状态
    */
    cxxui::json OnAppPin(cxxui::json& arg) {
        bool pin;
        if (!arg.contains("pin")) {
            LoadData();
            pin = data_.value("pin", false);
        } else {
            pin = arg.value("pin", false);
            data_["pin"] = pin;
            SaveData();
        }
        SetWindowPos(this->GetHandle(),
                     pin ? HWND_TOPMOST : HWND_NOTOPMOST,
                     0,
                     0,
                     0,
                     0,
                     SWP_NOMOVE | SWP_NOSIZE);
        return pin;
    }
    /*
    获取账号列表
    arg: null
    v2.0.0 以前(不含)
    ret: {name: string; dir: string}[]
    v2.0.0 以后(含)
    ret: {
            user: {name: string; dir: string}[];
            isMulti?: Boolean; // 是否处于多选模式
            selected?: number[]; // 已勾选的下标数组
        }
    */
    cxxui::json OnUserGet(cxxui::json&) {
        LoadData();
        return data_;
    }
    /*
    获取已经登录的账号
    arg: null
    ret: string[] 账号目录列表
    */
    static cxxui::json OnUserGetDirs(cxxui::json&) {
        cxxui::json dirs = WeiXin::GetWaitingDirs();
        dirs.get_ref<cxxui::json::array_t&>().reserve(dirs.size() + WeiXin::GetAllObjs().size());
        for (const auto& obj : WeiXin::GetAllObjs()) {
            dirs.emplace_back(obj->GetDir());
        }
        return dirs;
    }
    /*
    登录账号
    arg: { dir: string }[] 登录的账号目录
    ret: null
    */
    cxxui::json OnUserLogin(cxxui::json& arg) {
        if (!arg.is_array() || arg.empty()) {
            return nullptr;
        }
        Show(false);  // 避免闪烁
        for (const auto& item : arg) {
            WeiXin::Create(item.value("dir", ""));
        }
        win_ = nullptr;
        return nullptr;
    }
    /*
    添加或修改账号信息
    arg: {
            dir: string;   // 账号目录
            name?: string; // 昵称, 为空表示删除账号
        }
    ret: null
    */
    cxxui::json OnUserSet(cxxui::json& arg) {
        std::string name = arg.value("name", "");
        std::string dir = arg.value("dir", "");
        if (dir.empty()) {
            return nullptr;
        }
        auto it = std::find_if(
            data_["user"].begin(), data_["user"].end(), [&dir](const cxxui::json& user) {
                return user.at("dir").get<std::string_view>() == dir;
            });
        if (name.empty()) {
            // 删除账号信息
            if (it != data_["user"].end()) {
                data_["user"].erase(it);
            }
            // 删除磁盘目录
            namespace fs = std::filesystem;
            fs::path user_dir = Config::GetInstance().GetUserDir() / dir;
            if (fs::exists(user_dir)) {
                fs::remove_all(user_dir);
            }
        } else {
            // 更新或添加账号信息
            if (it != data_["user"].end()) {
                (*it)["name"] = name;  // 更新
                // 更新正在运行的微信
                const auto& objs = WeiXin::GetAllObjs();
                auto obj_it = std::find_if(objs.begin(), objs.end(), [&dir](const auto& obj) {
                    return obj->GetDir() == dir;
                });
                if (obj_it != objs.end()) {
                    (*obj_it)->UpdateTitle(name);
                }
            } else {
                data_["user"].emplace_back(arg);  // 添加
            }
        }
        SaveData();
        return nullptr;
    }
    /*
    修改多选模式参数
    arg: {isMulti?: Boolean; selected?: number[];}
    ret: null
    */
    cxxui::json OnUserSetMulti(cxxui::json& arg) {
        if (arg.contains("isMulti")) {
            data_["isMulti"] = arg.at("isMulti");
        }
        if (arg.contains("selected")) {
            data_["selected"] = arg.at("selected");
        }
        SaveData();
        return nullptr;
    }
    /*
    移动账号
    arg: {from: number; to: number;}
    ret: null
    */
    cxxui::json OnUserMove(cxxui::json& arg) {
        int from = arg.value("from", -1);
        int to = arg.value("to", -1);
        if (from < 0 || to < 0 || from == to) {
            return nullptr;
        }

        auto& users = data_["user"];
        auto size = static_cast<int>(users.size());
        if (from >= size || to >= size) {
            return nullptr;
        }
        std::swap(users[from], users[to]);
        // TODO(v3.0): selected 存储 dir 列表
        auto& selected = data_["selected"];
        auto from_it = std::find(selected.begin(), selected.end(), from);
        auto to_it = std::find(selected.begin(), selected.end(), to);
        if (from_it != selected.end() && to_it == selected.end()) {
            *from_it = to;
        } else if (to_it != selected.end() && from_it == selected.end()) {
            *to_it = from;
        }

        SaveData();
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
    static void Refreseh() noexcept {
        if (win_) {
            win_->GetWebView()->Reload();
        }
    }
    static bool IsOpen() noexcept { return win_ != nullptr; }

private:
    inline static std::unique_ptr<LoginWindow> win_;
};
