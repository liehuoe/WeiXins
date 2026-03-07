#pragma once

#include <fstream>
#include <shlobj.h>
#include <cxxui/win.hpp>
#include <cxxui/web_win/js_msg_map.hpp>
#include "weixin.hpp"

class JsMsg {
    constexpr static std::string_view kConfigFile = "list.json";
    static std::filesystem::path GetConfigDir() {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(NULL, buffer, MAX_PATH - 1);
        std::filesystem::path dir = cxxui::detail::W2U8(buffer);
        return dir.remove_filename() / "users";
    }
    static std::filesystem::path GetWeiXinDir() {
        PWSTR path = nullptr;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &path);
        if (FAILED(hr)) {
            return "";
        }
        std::filesystem::path dir = cxxui::detail::W2U8(path);
        CoTaskMemFree(path);
        return dir / "Documents/xwechat_files/all_users";
    }

public:
    JsMsg() {
        map_.bind("/user/get", OnUserGet);
        map_.bind("/user/login", OnUserLogin);
        map_.bind("/user/set", OnUserSet);
    }
    std::function<std::string(std::string)> GetHandler() const noexcept {
        return map_.GetHandler();
    }
    /** 要登录的微信的程序配置文件目录 */
    inline static std::string login_dir;
    /** 程序的配置文件目录 */
    inline const static std::filesystem::path kConfigPath = GetConfigDir();
    /** 微信的配置文件目录 */
    inline const static std::filesystem::path kWeiXinDir = GetWeiXinDir();
    /** 更新配置文件到微信/程序 */
    static void UpdateConfig(std::string_view cfg_item, bool to_weixin = true) {
        namespace fs = std::filesystem;
        fs::path cfg_dir = kConfigPath / cfg_item;
        if (!fs::exists(cfg_dir)) {
            fs::create_directories(cfg_dir);
        }
        constexpr std::string_view kFileList[] = {"global_config", "global_config.crc"};
        fs::path src_dir, dst_dir;
        if (to_weixin) {
            src_dir = cfg_dir;
            dst_dir = GetWeiXinDir() / "config";
        } else {
            src_dir = GetWeiXinDir() / "config";
            dst_dir = cfg_dir;
        }
        for (const auto file : kFileList) {
            fs::path src = src_dir / file;
            fs::path dst = dst_dir / file;
            if (fs::exists(src)) {
                CopyFileExW(src.c_str(), dst.c_str(), nullptr, nullptr, nullptr, 0);
            } else {
                fs::remove(dst);
            }
        }
    }

private:
    cxxui::JsMsgMap<> map_;
    static cxxui::json OnUserGet(cxxui::json&) {
        return cxxui::json::parse(std::ifstream{kConfigPath / kConfigFile});
    }

    static cxxui::json OnUserLogin(cxxui::json& arg) {
        namespace fs = std::filesystem;
        // 拷贝用户配置
        login_dir = arg.at("dir").get<std::string>();
        UpdateConfig(login_dir);
        // 删除头像目录
        fs::path head_imgs = kWeiXinDir / "head_imgs";
        if (fs::exists(head_imgs)) {
            fs::remove_all(kWeiXinDir / "head_imgs");
        }
        cxxui::Exit(100);  // 返回100代表登录
        return nullptr;
    }
    static cxxui::json OnUserSet(cxxui::json& arg) {
        namespace fs = std::filesystem;
        cxxui::json users;
        for (const auto& user : arg) {
            if (user.contains("name") && !user.at("name").empty()) {
                users.emplace_back(user);
                continue;
            }
            // 删除账号目录
            fs::path user_dir = kConfigPath / user.at("dir").get<std::string>();
            if (fs::exists(user_dir)) {
                fs::remove_all(user_dir);
            }
        }
        if (!fs::exists(kConfigPath)) {
            fs::create_directories(kConfigPath);
        }
        std::ofstream{kConfigPath / kConfigFile} << users.dump(4);
        return nullptr;
    }
};
