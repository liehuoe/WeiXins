#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <config.hpp>
#include "weixin/hotkey.hpp"
#include <gdiplus.h>

class WeiXin : public weixin::HotKey<WeiXin> {
    using Base = weixin::HotKey<WeiXin>;

public:
    /**
     * @brief 创建微信进程，启动微信
     *
     * @return WeiXin* 每个微信进程对应一个微信对象，指针在OnClose中释放
     */
    static WeiXin* Create(std::string bak_dir) {
        namespace fs = std::filesystem;
        // 复制登录文件到微信
        weixin::CopyLoginFiles<true>(Config::GetInstance().GetUserDir() / bak_dir);
        // 删除微信缓存的头像目录
        fs::path head_imgs = weixin::GetHeadImgDir();
        if (fs::exists(head_imgs)) {
            fs::remove_all(head_imgs);
        }
        // 启动微信
        auto wx = new WeiXin{std::move(bak_dir)};
        if (!wx->Start()) {
            delete wx;
            MessageBoxW(nullptr, L"微信启动失败", L"微信多开助手", MB_OK | MB_ICONERROR);
            return nullptr;
        }
        wx->CheckLogin();

        return wx;
    }
    /**
     * @brief 检测是否所有微信都已经退出
     *
     * @return bool
     */
    static bool IsAllCloseed() noexcept { return Base::IsAllCloseed(); }
    /**
     * @brief 获取所有已经启动的微信
     *
     * @return const std::vector<WeiXin*>&
     */
    static const std::vector<WeiXin*>& GetAllObjs() noexcept { return Base::GetAllObjs(); }
    /**
     * @brief 获取登录文件的备份目录
     *
     * @return const std::string&
     */
    const std::string& GetDir() const noexcept { return dir_; }

protected:
    friend class weixin::CloseChecker<WeiXin>;
    /** 微信退出时触发 */
    void OnClose(HANDLE handle) {
        Base::OnClose(handle);
        delete this;
    }

    friend class weixin::LoginChecker<WeiXin>;
    /** 微信登录成功时触发 */
    void OnLogin(HWND hwnd) {
        Base::OnLogin(hwnd);

        weixin::CopyLoginFiles<false>(dir_);  // 更新登录文件
        weixin::CopyHeadImg(dir_);            // 更新头像
        UpdateTitle();                        // 更新任务栏标题
        UpdateIcon();                         // 更新任务栏图标
    }

private:
    std::string dir_;
    WeiXin(std::string dir)
        : dir_(std::move(dir)) {
        if (!gdi_token_) {
            Gdiplus::GdiplusStartupInput startupInput;
            startupInput.GdiplusVersion = 1;
            Gdiplus::GdiplusStartup(&gdi_token_, &startupInput, nullptr);
        }
    }

    /** 更新微信任务栏的标题 */
    void UpdateTitle() {
        auto root = nlohmann::json::parse(std::ifstream{Config::GetInstance().GetCfgPath()});
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
        SetWindowTextW(GetMainHwnd(), cxxui::detail::U82W(login_name).c_str());
    }
    struct Icon {
        HICON value = nullptr;
        ~Icon() {
            if (value) {
                DestroyIcon(value);
            }
        }
    };
    inline static ULONG_PTR gdi_token_ = 0;
    Icon icon_;
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
        icon_.value = icon;
        LPARAM lp = reinterpret_cast<LPARAM>(icon);
        PostMessageW(GetMainHwnd(), WM_SETICON, ICON_BIG, lp);
        PostMessageW(GetMainHwnd(), WM_SETICON, ICON_SMALL, lp);
    }
};
