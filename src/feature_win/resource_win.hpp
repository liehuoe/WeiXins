#pragma once

#include <charconv>
#include <cxxui/web_win.hpp>
#include "resource.h"
#include "config.hpp"

#ifdef _DEBUG
    // debug 模式 直接访问本地前端的调试网址
    #define PAGE(name) "http://localhost:5173?page=" #name
#else
    // release 模式的请求全部由 OnWebRequest 拦截并处理
    // 1000 表示 html 文件的资源ID, 与 resource.rc 中的定义对应
    #define PAGE(name) "http://./1000.html?page=" #name
#endif

/** 通过程序内置资源响应 webrequest 请求 */
template <typename Derived>
class ResourceWindow : public cxxui::WebWindow<Derived> {
    using Base = cxxui::WebWindow<Derived>;

public:
    using Base::Base;

protected:
    CXXUI_WEB_EVENT(Derived)
    void OnWebCreated(std::optional<cxxui::WindowError> err) {
#ifndef _DEBUG
        // *.head_img 的请求使用本地保存的头像文件进行响应
        // 文件名为 logo 表示请求读取程序本身的图标
        // 文件名为 数字 表示请求对应资源ID的资源文件
        this->OnWebRequest([](cxxui::WebRequest& ctx) {
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
                        ctx.SetResponse(
                            (Config::GetInstance().GetUserDir() / name / kHeadImgName).u8string());
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
        // *.head_img 的请求使用本地保存的头像文件进行响应
        OnWebRequest(
            [](cxxui::WebRequest& ctx) {
                auto url = ctx.GetUrl();
                std::string_view urlv = url;
                auto pos = urlv.find_last_of('/') + 1;
                std::string dir{urlv.substr(pos, urlv.find(".head_img", pos + 1) - pos)};
                try {
                    ctx.SetResponse(
                        (Config::GetInstance().GetUserDir() / dir / kHeadImgName).u8string());
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
        Base::OnWebCreated(err);
    }
};
