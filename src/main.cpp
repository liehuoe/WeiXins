#include <winsock2.h>
#include <fstream>
#include <charconv>
#include <cxxui/web_win.hpp>
#include "resource.h"
#include "js_msg.hpp"

int ShowLoginDlg() {
    cxxui::WebWindow win{
        cxxui::WindowOptions().SetTitle("微信多开助手").SetWidth(300).SetHeight(400)};
    win.SetIcon(IDI_LOGO);
    win.WaitWebCreated();

    JsMsg msg;
    win.SetJsMsgHandler(msg.GetHandler());
#ifndef _DEBUG
    // 处理资源请求
    win.SetRequestHandler([](cxxui::RequestContext& ctx) {
        auto url = ctx.GetUrl();
        std::string_view urlv = url;
        urlv.remove_prefix((std::min)(9, static_cast<int>(urlv.size())));  // http://./
        auto pos = urlv.find('.');
        auto name = std::string{urlv.substr(0, pos)};

        if (pos != std::string_view::npos) {
            // 根据扩展名设置 Content-Type
            urlv.remove_prefix(pos + 1);
            std::string_view ext = urlv.substr(0, urlv.find_first_of("/?#"));
            // 加载微信头像
            if (ext == "head_img") {
                try {
                    ctx.SetHeaders(std::string{ctx.GetContentType("jpg")});
                    ctx.SetResponse((JsMsg::kConfigPath / name / "logo.jpg").u8string());
                    return;
                } catch (...) {
                    name = "logo";  // 退化为加载默认logo
                }
            } else {
                ctx.SetHeaders(std::string{ctx.GetContentType(ext)});
            }
        }

        UINT rc_id = 0;
        if (name == "logo") {
            rc_id = IDD_LOGO;
        } else {
            std::from_chars(name.data(), name.data() + name.size(), rc_id);
        }
        HINSTANCE hinst = GetModuleHandle(nullptr);
        HRSRC hr = FindResource(hinst, MAKEINTRESOURCE(rc_id), RT_RCDATA);
        if (!hr) {
            ctx.SetResponse(404);
            return;
        }
        DWORD size = SizeofResource(hinst, hr);
        if (size <= 0) {
            ctx.SetResponse(404);
            return;
        }
        HGLOBAL hg = LoadResource(hinst, hr);
        if (!hg) {
            ctx.SetResponse(404);
            return;
        }
        ctx.SetResponse(LockResource(hg), size);
    });
#else
    // 处理资源请求
    win.SetRequestHandler(
        [](cxxui::RequestContext& ctx) {
            auto url = ctx.GetUrl();
            std::string_view urlv = url;
            auto pos = urlv.find_last_of('/') + 1;
            std::string dir{urlv.substr(pos, urlv.find(".head_img", pos + 1) - pos)};
            try {
                ctx.SetResponse((JsMsg::kConfigPath / dir / "logo.jpg").u8string());
                ctx.SetHeaders(std::string{ctx.GetContentType("jpg")});
            } catch (...) {
                HINSTANCE hinst = GetModuleHandle(nullptr);
                HRSRC hr = FindResource(hinst, MAKEINTRESOURCE(IDD_LOGO), RT_RCDATA);
                if (!hr) {
                    ctx.SetResponse(404);
                    return;
                }
                DWORD size = SizeofResource(hinst, hr);
                if (size <= 0) {
                    ctx.SetResponse(404);
                    return;
                }
                HGLOBAL hg = LoadResource(hinst, hr);
                if (!hg) {
                    ctx.SetResponse(404);
                    return;
                }
                ctx.SetResponse(LockResource(hg), size);
            }
        },
        "*.head_img");
#endif

    win.SetUrl(PAGE(login));
    win.Show();
    return win.Run();
}

std::filesystem::path GetFirstFile(const std::filesystem::path& dir,
                                   std::string_view exclude = {}) {
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (!exclude.empty() && entry.path().filename() == exclude) {
            continue;
        }
        return entry.path();
    }
    return {};
}
void LaunchWeiXin() {
    // 启动微信
    DWORD pid = 0;
    try {
        WeiXin weixin;
        pid = weixin.Start();
    } catch (...) {
        // pid = 0
    }
    if (pid == 0) {
        MessageBoxW(nullptr, L"无法启动微信", L"系统提示", MB_OK);
        return;
    }
    struct CallbackData {
        DWORD pid;
        DWORD count;
    };
    // 等待微信登录
    while (1) {
        Sleep(1000);

        CallbackData data{pid, 0};
        EnumWindows(
            [](HWND hwnd, LPARAM lp) -> BOOL {
                CallbackData* data = reinterpret_cast<CallbackData*>(lp);
                DWORD pid = 0;
                GetWindowThreadProcessId(hwnd, &pid);
                if (pid == data->pid) {
                    data->count++;
                }
                return TRUE;
            },
            reinterpret_cast<LPARAM>(&data));
        if (data.count == 0) {
            return;  // 微信退出
        } else if (data.count <= 5) {
            continue;  // 微信登录中
        }
        break;
    }
    try {
        namespace fs = std::filesystem;
        // 更新微信头像
        fs::path logo_dir = GetFirstFile(JsMsg::kWeiXinDir / "head_imgs", "0");
        if (!logo_dir.empty()) {
            fs::path logo_src = GetFirstFile(logo_dir);
            if (!logo_src.empty()) {
                fs::path logo_dst = JsMsg::kConfigPath / JsMsg::login_dir / "logo.jpg";
                CopyFileExW(logo_src.c_str(), logo_dst.c_str(), nullptr, nullptr, nullptr, 0);
            }
        }
        // 更新配置文件
        JsMsg::UpdateConfig(JsMsg::login_dir, false);
    } catch (...) {
        // ignore
    }
}

int main() {
    try {
        int ret = ShowLoginDlg();
        if (ret == 100) {
            LaunchWeiXin();
        }
    } catch (const std::exception& e) {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK);
    }
    return 0;
}
