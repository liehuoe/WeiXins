#pragma once

#include <cxxui/core/detail/library.hpp>

/**
 * @brief 获取窗口句柄的DPI
 *
 * @param hwnd 窗口句柄
 * @return UINT DPI值
 */
inline UINT GetDpi(HWND hwnd) {
    UINT dpi = 96;
    using GetDpiForWindowFunc = UINT(WINAPI*)(HWND);
    cxxui::detail::Library lib("user32.dll");
    if (auto func = lib.Get<GetDpiForWindowFunc>("GetDpiForWindow"); func) {
        dpi = func(hwnd);
    } else {
        HDC hdc = GetDC(nullptr);
        if (hdc) {
            dpi = GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(nullptr, hdc);
        }
    }
    return dpi;
}

/**
 * @brief 判断字符串 str 是否以 end 结尾
 *
 * @param str 要查找的字符串
 * @param end 要匹配的子字符串
 */
inline bool EndWith(std::string_view str, std::string_view end) {
    return str.size() >= end.size() && str.substr(str.size() - end.size()) == end;
}

/**
 * @brief 设置窗口到前台
 *
 * @param hwnd 要设置的窗口句柄
 */
inline void SetForeground(HWND hwnd) {
    DWORD fore_id = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
    DWORD cur_id;
    GetWindowThreadProcessId(hwnd, &cur_id);

    AttachThreadInput(cur_id, fore_id, TRUE);

    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    } else {
        ShowWindow(hwnd, SW_SHOW);
    }
    SetForegroundWindow(hwnd);

    AttachThreadInput(cur_id, fore_id, FALSE);
}

/**
 * @brief
 *
 */
template <typename Derived>
class Singleton {
protected:
    static Derived& GetInstance() {
        static Derived instance;
        return instance;
    }
};

