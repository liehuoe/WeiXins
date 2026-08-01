#include "app.hpp"
#include "login_win.hpp"

#if defined(_MSC_VER)
    #include <dbghelp.h>
    #pragma comment(lib, "dbghelp.lib")
LONG WINAPI GenerateDumpAndContinue(EXCEPTION_POINTERS* ep) {
    wchar_t path[MAX_PATH];
    SYSTEMTIME st;
    GetLocalTime(&st);
    swprintf_s(path,
               _countof(path),
               L"crash_%04d%02d%02d_%02d%02d%02d_%03d.dmp",
               st.wYear,
               st.wMonth,
               st.wDay,
               st.wHour,
               st.wMinute,
               st.wSecond,
               st.wMilliseconds);
    HANDLE file =
        CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei = {};
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = ep;  // ← 传入原始异常指针
        mei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(),
                          GetCurrentProcessId(),
                          file,
                          MiniDumpWithFullMemory,
                          &mei,
                          NULL,
                          NULL);
        CloseHandle(file);
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

int Run();
int main() {
    __try {
        return Run();
    } __except (GenerateDumpAndContinue(GetExceptionInformation())) {
        MessageBoxW(nullptr, L"程序意外退出", PROJECT_DESC_W, MB_OK | MB_ICONERROR);
    }
}

int Run()
#else
int main()
#endif
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    // 判断是否已经有其他的app进程正在运行，必须在app初始化前调用
    if (App::IsRunning([]() { LoginWindow::Open(); })) {
        return 1;
    }
    App& app = App::GetInstance();  // 初始化并获取主窗口实例对象

    LoginWindow::Open();  // 打开登录窗口
    auto ret = app.Run([]() {
        if (!WeiXin::IsAllCloseed()) {
            return;
        }
        if (LoginWindow::IsOpen()) {
            return;
        }
        cxxui::Exit();
    });
    weixin::CleanLoginFiles();

    CoUninitialize();
    return ret;
}
