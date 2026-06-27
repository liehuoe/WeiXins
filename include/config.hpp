#pragma once

#include <filesystem>
#include <windows.h>

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

private:
    Config() {
        wchar_t buffer[MAX_PATH]{};
        GetModuleFileNameW(NULL, buffer, MAX_PATH - 1);
        user_dir_ = std::filesystem::path{buffer}.remove_filename() / "users";
    }
    std::filesystem::path user_dir_;
};
