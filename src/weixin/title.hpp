#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <config.hpp>
#include "hotkey.hpp"
#include <gdiplus.h>

namespace weixin {

struct Icon {
    HICON value = nullptr;
    ~Icon() {
        if (value) {
            DestroyIcon(value);
        }
    }
};

template <typename Derived>
class Title : public HotKey<Derived> {
    using Base = HotKey<Derived>;

public:
    /**
     * @brief 获取登录文件的备份目录
     *
     * @return const std::string&
     */
    const std::string& GetDir() const noexcept { return dir_; }
    /**
     * @brief 更新任务栏标题
     *
     * @param name 新标题
     */
    void UpdateTitle(std::string_view name) {
        SetWindowTextW(this->GetMainHwnd(), cxxui::detail::U82W(name).c_str());
    }

protected:
    Title(std::string dir)
        : dir_(std::move(dir)) {
        if (!gdi_token_) {
            Gdiplus::GdiplusStartupInput startup_input;
            startup_input.GdiplusVersion = 1;
            Gdiplus::GdiplusStartup(&gdi_token_, &startup_input, nullptr);
        }
    }
    friend class weixin::LoginChecker<Derived>;
    /** 微信登录成功时触发 */
    void OnLogin(HWND hwnd) {
        Base::OnLogin(hwnd);

        auto dir_path = Config::GetInstance().GetUserDir() / dir_;
        weixin::CopyLoginFiles(dir_path, false);  // 更新登录文件
        weixin::CopyHeadImg(dir_path);            // 更新头像
        UpdateTitle();                            // 更新任务栏标题
        UpdateIcon();                             // 更新任务栏图标
    }

private:
    std::string dir_;
    Icon icon_;
    inline static ULONG_PTR gdi_token_ = 0;

    /** 更新微信任务栏的标题 */
    void UpdateTitle() {
        auto root = nlohmann::json::parse(std::ifstream{Config::GetInstance().GetCfgPath()});
        if (!root.is_array() && !root.contains("user")) {
            return;
        }
        root = root["user"];
        if (!root.is_array()) {
            return;
        }
        std::string login_name;
        for (const auto& item : root) {
            std::string name = item.value("name", "");
            std::string dir = item.value("dir", "");
            if (name.empty() || dir.empty()) {
                continue;
            }
            if (dir != dir_) {
                continue;
            }
            login_name = std::move(name);
            break;
        }
        if (login_name.empty()) {
            return;
        }
        UpdateTitle(login_name);
    }
    /** 更新微信任务栏的图标 */
    void UpdateIcon() {
        // 加载图片
        auto jpg_path = Config::GetInstance().GetUserDir() / dir_ / weixin::kHeadImgName;
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
        constexpr int radius = 100;
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
        icon_.value = icon;
        LPARAM lp = reinterpret_cast<LPARAM>(icon);
        SendMessageW(this->GetMainHwnd(), WM_SETICON, ICON_BIG, lp);
        SendMessageW(this->GetMainHwnd(), WM_SETICON, ICON_SMALL, lp);
    }
};

}  // namespace weixin
