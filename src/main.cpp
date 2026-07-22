#include "app.hpp"
#include "login_win.hpp"

int main() {
    // 判断是否已经有其他的app进程正在运行，必须在app初始化前调用
    if (App::IsRunning([]() { LoginWindow::Open(); })) {
        return 1;
    }
    App& app = App::GetInstance();  // 初始化并获取主窗口实例对象
    LoginWindow::Open();            // 打开登录窗口
    return app.Run([]() {
        if (!WeiXin::IsAllCloseed()) {
            return;
        }
        if (LoginWindow::IsOpen()) {
            return;
        }
        weixin::CleanLoginFiles();
        cxxui::Exit();
    });
}