#pragma once

#include "./resource_win.hpp"
#include <dwmapi.h>

/** 处理浅深色背景 */
template <typename Derived>
class DarkWindow : public ResourceWindow<Derived> {
    using Base = ResourceWindow<Derived>;

public:
    DarkWindow(cxxui::WindowOptions opts)
        : Base(std::move(opts.SetWin32Style(WS_OVERLAPPED | WS_SIZEBOX | WS_MAXIMIZEBOX))) {
        UseDarkMode<true>();
    }

protected:
    CXXUI_WIN_EVENT(Derived)
    std::optional<LRESULT> OnWin32Msg(UINT msg, WPARAM wp, LPARAM lp) {
        switch (msg) {
            case WM_ERASEBKGND: {
                HDC hdc = reinterpret_cast<HDC>(wp);
                RECT rc;
                GetClientRect(this->hwnd_, &rc);
                FillRect(hdc, &rc, bg_brush_);
                return TRUE;
            }
            case WM_CREATE: {
                SetWindowPos(
                    this->hwnd_, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);
                break;
            }
            case WM_NCCALCSIZE: {
                if (wp == TRUE) {
                    NCCALCSIZE_PARAMS* params = reinterpret_cast<NCCALCSIZE_PARAMS*>(lp);
                    params->rgrc[0].top -= GetTitleHeight();
                }
                break;
            }
            default:
                break;
        }
        return Base::OnWin32Msg(msg, wp, lp);
    }
    int GetTitleHeight() noexcept {
        static cxxui::detail::Library lib{"user32.dll"};
        using AdjustWindowRectExForDpiPtr = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
        using GetDpiForWindowPtr = UINT(WINAPI*)(HWND hwnd);
        auto AdjustWindowRectExForDpi =
            lib.Get<AdjustWindowRectExForDpiPtr>("AdjustWindowRectExForDpi");
        auto GetDpiForWindow = lib.Get<GetDpiForWindowPtr>("GetDpiForWindow");
        if (!AdjustWindowRectExForDpi || !GetDpiForWindow) {
            return GetSystemMetrics(SM_CYCAPTION);
        }
        RECT frame{};
        DWORD style = GetWindowLongW(this->hwnd_, GWL_STYLE);
        auto dpi = GetDpiForWindow(this->hwnd_);
        AdjustWindowRectExForDpi(&frame, style, FALSE, 0, dpi);
        return -frame.top;
    }
    void OnSetting(cxxui::SettingEvent& event) {
        if (event.IsColorThemeChanged()) {
            UseDarkMode();
        }
        Base::OnSetting(event);
    }

private:
    bool is_dark_;
    class Brush {
    public:
        ~Brush() { Reset(); }
        operator HBRUSH() const { return brush_; }
        void SetColor(cxxui::Color color) {
            brush_ = CreateSolidBrush(RGB(color.red, color.green, color.blue));
        }

    private:
        void Reset() {
            if (brush_) {
                DeleteObject(brush_);
                brush_ = nullptr;
            }
        }
        HBRUSH brush_ = nullptr;
    };
    Brush bg_brush_;
    template <bool force = false>
    void UseDarkMode() noexcept {
        bool is_dark = cxxui::IsDarkMode();
        if constexpr (!force) {
            if (is_dark_ == is_dark) {
                return;
            }
        }
        is_dark_ = is_dark;
        auto color = is_dark_ ? cxxui::Color{18, 18, 18} : cxxui::Color{255, 255, 255};
        bg_brush_.SetColor(color);
        InvalidateRect(this->hwnd_, nullptr, TRUE);
    }
};
