#pragma once

#include <cxxui/web_win.hpp>
#include "resource.h"

class WeiXinWindow : public cxxui::WebWindow<WeiXinWindow> {
public:
    WeiXinWindow(cxxui::WindowOptions opts)
        : WebWindow(std::move(opts)) {
        SetIcon(IDI_LOGO);
        is_dark_ = cxxui::IsDarkMode();
        UseDarkMode();
    }
    ~WeiXinWindow() {
        if (bg_brush_) {
            DeleteObject(bg_brush_);
        }
    }
    void OnSetting(const cxxui::SettingEvent& event) {
        if (event.IsColorThemeChanged()) {
            bool is_dark = cxxui::IsDarkMode();
            if (is_dark_ != is_dark) {
                is_dark_ = is_dark;
                UseDarkMode();
            }
        }
        cxxui::WebWindow<WeiXinWindow>::OnSetting(event);
    }
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_ERASEBKGND: {
                HDC hdc = reinterpret_cast<HDC>(wp);
                RECT rc;
                GetClientRect(this->hwnd_, &rc);
                FillRect(hdc, &rc, bg_brush_);
                return TRUE;
            }
            default:
                break;
        }
        return cxxui::WebWindow<WeiXinWindow>::OnWin32Msg(msg, wp, lp);
    }
    void UseDarkMode() noexcept {
        auto color = is_dark_ ? cxxui::Color{18, 18, 18} : cxxui::Color{255, 255, 255};
        try {
            SetTitleColor(color);
        } catch (...) {
            // ignore
        }
        if (bg_brush_) {
            DeleteObject(bg_brush_);
        }
        bg_brush_ = CreateSolidBrush(RGB(color.red, color.green, color.blue));
        InvalidateRect(this->hwnd_, nullptr, TRUE);
    }

private:
    bool is_dark_;
    HBRUSH bg_brush_ = nullptr;
};
