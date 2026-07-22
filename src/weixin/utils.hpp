#pragma once

#include <filesystem>
#include <windows.h>
#include <Shlobj.h>
#include <utils.hpp>
#include "config.h"

namespace weixin {

/**
 * @brief 获取微信的用户数据目录
 *
 * @return std::filesystem::path 获取失败返回空路径
 */
inline std::filesystem::path GetProfileDir() noexcept {
    std::filesystem::path path;
    PWSTR profile_dir = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Profile, 0, nullptr, &profile_dir))) {
        path = std::filesystem::path{profile_dir} / "Documents/xwechat_files/all_users";
        CoTaskMemFree(profile_dir);
    }
    return path;
}

/**
 * @brief 获取微信的登录配置文件所在的目录
 *
 * @return std::filesystem::path 获取失败返回空路径
 */
inline std::filesystem::path GetConfigDir() noexcept {
    std::filesystem::path path = GetProfileDir();
    if (!path.empty()) {
        path /= "config";
    }
    return path;
}

/**
 * @brief 获取微信的头像文件所在的目录
 *
 * @return std::filesystem::path 获取失败返回空路径
 */
inline std::filesystem::path GetHeadImgDir() noexcept {
    std::filesystem::path path = GetProfileDir();
    if (!path.empty()) {
        path /= "head_imgs";
    }
    return path;
}

namespace detail {

inline void SafeRemove(const std::filesystem::path& dir, std::string_view file) {
    namespace fs = std::filesystem;
    std::filesystem::path file_path = dir / file;
    std::error_code ec;
    fs::remove(file_path, ec);
    if (!ec) {
        return;
    }
    // del_dir 需要在程序退出的时候清理删除
    fs::path del_dir = dir / PROJECT_NAME;
    if (!fs::exists(del_dir, ec)) {
        fs::create_directories(del_dir, ec);
        if (ec) {
            Debug("create %ls failed: %s\n", del_dir.c_str(), ec.message().c_str());
            return;
        }
    }
    auto del_file = std::string{file} + std::to_string(GetTickCount());
    fs::rename(file_path, del_dir / del_file, ec);
    if (ec) {
        Debug("rename %ls failed: %s\n", file_path.c_str(), ec.message().c_str());
    }
}

}  // namespace detail

/**
 * @brief 复制登录配置文件到微信/程序
 *
 * @tparam to_weixin true: 从 dir 更新到微信, false: 从微信更新到 dir
 * @param bak_dir 登录配置文件备份的目录
 */
template <bool to_weixin = true>
void CopyLoginFiles(const std::filesystem::path& bak_dir) {
    namespace fs = std::filesystem;
    if (!fs::exists(bak_dir)) {
        fs::create_directories(bak_dir);
    }
    auto wx_dir = GetConfigDir();
    if (wx_dir.empty()) {
        return;
    }
    fs::path src_dir, dst_dir;
    if constexpr (to_weixin) {
        src_dir = bak_dir;
        dst_dir = wx_dir;
    } else {
        src_dir = wx_dir;
        dst_dir = bak_dir;
    }
    constexpr std::string_view kFileList[] = {"global_config", "global_config.crc"};
    for (const auto file : kFileList) {
        fs::path src = src_dir / file;
        fs::path dst = dst_dir / file;
        detail::SafeRemove(dst_dir, file);
        std::error_code ec;
        if (fs::exists(dst)) {
            if constexpr (to_weixin) {
                detail::SafeRemove(dst_dir, file);
            } else {
                fs::remove(dst, ec);
                if (ec) {
                    Debug("remove %ls failed: %s\n", dst.c_str(), ec.message().c_str());
                }
            }
        }
        if (fs::exists(src)) {
            CopyFileExW(src.c_str(), dst.c_str(), nullptr, nullptr, nullptr, 0);
        }
    }
}

/**
 * @brief 清理多余的登录文件
 */
inline void CleanLoginFiles() {
    namespace fs = std::filesystem;

    auto wx_dir = GetConfigDir();
    if (wx_dir.empty()) {
        return;
    }
    wx_dir /= PROJECT_NAME;

    std::error_code ec;
    if (fs::exists(wx_dir, ec)) {
        fs::remove_all(wx_dir, ec);
        if (ec) {
            Debug("CleanLoginFiles failed: %s\n", ec.message().c_str());
        }
    }
}

/**
 * @brief 微信头像文件在程序备份目录中的文件名
 */
inline std::string_view kHeadImgName = "logo.jpg";
/**
 * @brief 复制微信头像文件到程序
 *
 * @param bak_dir 程序的备份目录
 */
inline void CopyHeadImg(const std::filesystem::path& bak_dir) {
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
    fs::path logo_dir = GetFirstFile(GetHeadImgDir(), "0");
    if (!logo_dir.empty()) {
        fs::path logo_src = GetFirstFile(logo_dir);
        if (!logo_src.empty()) {
            fs::path logo_dst = bak_dir / kHeadImgName;
            CopyFileExW(logo_src.c_str(), logo_dst.c_str(), nullptr, nullptr, nullptr, 0);
        }
    }
}

/**
 * @brief 获取微信托盘图标窗口
 *
 * @param pid 进程ID
 * @return HWND 返回托盘图标窗口句柄
 */
inline HWND GetTrayWindow(DWORD pid) {
    struct EnumData {
        DWORD pid;
        HWND hwnd;
    };
    EnumData data{pid, nullptr};
    EnumWindows(
        [](HWND hwnd, LPARAM lp) -> BOOL {
            EnumData* pdata = reinterpret_cast<EnumData*>(lp);
            DWORD cur_pid = 0;
            GetWindowThreadProcessId(hwnd, &cur_pid);
            if (cur_pid != pdata->pid) {
                return TRUE;
            }
            wchar_t name[256];
            GetClassNameW(hwnd, name, sizeof(name));
            if (!EndWith(name, L"WxTrayIconMessageWindowClass")) {
                return TRUE;
            }
            pdata->hwnd = hwnd;
            return FALSE;
        },
        reinterpret_cast<LPARAM>(&data));
    return data.hwnd;
}

}  // namespace weixin