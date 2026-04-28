#pragma once

#include <cxxui/web_win.hpp>

struct Counter {
    inline static int value = 0;
};

/** 通过程序内置资源响应 webrequest 请求 */
template <typename Derived>
class CountWindow : public cxxui::WebWindow<Derived> {
    using Base = cxxui::WebWindow<Derived>;

public:
    using Base::Base;
    static int Count() { return Counter::value; }

protected:
    CXXUI_WIN_EVENT(Derived)
    void OnCreated() {
        ++Counter::value;
        Base::OnCreated();
    }
    void OnClosed() {
        --Counter::value;
        Base::OnClosed();
    }
};
