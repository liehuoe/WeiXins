#pragma once

#include <cxxui/core/detail/library.hpp>

/**
 * @brief 获取窗口句柄的DPI
 *
 * @param hwnd 窗口句柄
 * @return UINT DPI值
 */
UINT GetDpi(HWND hwnd) {
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
bool EndWith(std::string_view str, std::string_view end) {
    return str.size() >= end.size() && str.substr(str.size() - end.size()) == end;
}