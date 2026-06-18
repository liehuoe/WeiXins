#pragma once

#include <filesystem>
#include <windows.h>
#include <Shlobj.h>

/** 微信头像文件名 */
inline std::string_view kHeadImgName = "logo.jpg";

class Config {
public:
    static Config& GetInstance() {
        static Config instance;
        return instance;
    }
    /** 程序的用户数据存放目录 */
    const std::filesystem::path& GetUserDir() const noexcept { return user_dir_; }
    /** 程序的配置文件 */
    std::filesystem::path GetCfgPath() const noexcept { return user_dir_ / "list.json"; }
    /** 微信的用户数据存放目录 */
    const std::filesystem::path& GetWeiXinDir() const noexcept { return wx_dir_; }
    /** 微信的头像存放目录 */
    std::filesystem::path GetHeadImgDir() const noexcept { return wx_dir_ / "head_imgs"; }

private:
    Config() {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(NULL, buffer, MAX_PATH - 1);
        user_dir_ = std::filesystem::path{buffer}.remove_filename() / "users";

        PWSTR profile_dir = nullptr;
        HRESULT hr = SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &profile_dir);
        if (SUCCEEDED(hr)) {
            wx_dir_ = std::filesystem::path{profile_dir} / "Documents/xwechat_files/all_users";
            CoTaskMemFree(profile_dir);
        }
    }
    std::filesystem::path user_dir_;
    std::filesystem::path wx_dir_;
};
