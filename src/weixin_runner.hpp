#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <windows.h>
#include <tchar.h>
#include <aclapi.h>

#pragma comment(lib, "Advapi32.lib")

class ProcessData {
public:
    ProcessData(DWORD pid, HANDLE process, HANDLE thread)
        : pid_(pid),
          process_(process),
          thread_(thread) {}
    ~ProcessData() {
        CloseHandle(process_);
        CloseHandle(thread_);
    }
    DWORD GetPid() const noexcept { return pid_; }
    HANDLE GetProcess() const noexcept { return process_; }
    HANDLE GetThread() const noexcept { return thread_; }

private:
    DWORD pid_;
    HANDLE process_;
    HANDLE thread_;
};

class WeiXinRunner {
public:
#ifdef UNICODE
    using String = std::wstring;
#else
    using String = std::string;
#endif
    WeiXinRunner() {
        struct RegKey {
            HKEY value;
            RegKey() {
                LSTATUS rc = RegOpenKeyEx(
                    HKEY_CURRENT_USER, _T("SOFTWARE\\Tencent\\Weixin"), 0, KEY_READ, &value);
                if (rc != ERROR_SUCCESS) {
                    throw std::runtime_error("RegOpenKeyEx failed");
                }
            }
            ~RegKey() { RegCloseKey(value); }

        } key;
        TCHAR path[MAX_PATH] = {0};
        DWORD size = MAX_PATH;
        LSTATUS rc = RegQueryValueEx(key.value, _T("InstallPath"), NULL, NULL, (LPBYTE)path, &size);
        if (rc != ERROR_SUCCESS) {
            throw std::runtime_error("RegQueryValueEx failed");
        }
        path_ = path;
        if (!path_.empty() && path_.back() != _T('\\') && path_.back() != _T('/')) {
            path_ += _T('\\');
        }
        path_ += _T("Weixin.exe");
    }
    /**
     * @brief 获取微信执行路径
     *
     * @return const String&
     */
    const String& Path() const noexcept { return path_; }
    /**
     * @brief 解除微信多开的限制
     *
     */
    void UnlimitSignle() {
        struct Mutex {
            HANDLE value;
            Mutex() {
                value = CreateMutex(nullptr, FALSE, _T("XWeChat_App_Instance_Identity_Mutex_Name"));
                if (!value) {
                    throw std::runtime_error("CreateMutex failed");
                }
            }
            ~Mutex() { CloseHandle(value); }
        } mutex;
        struct Sid {
            PSID value;
            Sid() {
                SID_IDENTIFIER_AUTHORITY sid_auth = SECURITY_WORLD_SID_AUTHORITY;
                if (!AllocateAndInitializeSid(
                        &sid_auth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &value)) {
                    throw std::runtime_error("AllocateAndInitializeSid failed");
                }
            }
            ~Sid() { FreeSid(value); }
        } sid;
        struct Acl {
            PACL value;
            static DWORD Size() { return sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE); };
            Acl() { value = (PACL)LocalAlloc(LPTR, Size()); }
            ~Acl() { LocalFree(value); }
        } acl;
        if (!InitializeAcl(acl.value, acl.Size(), ACL_REVISION)) {
            throw std::runtime_error("InitializeAcl failed");
        }
        if (!AddAccessDeniedAce(acl.value, ACL_REVISION, MUTEX_ALL_ACCESS, sid.value)) {
            throw std::runtime_error("AddAccessDeniedAce failed");
        }
        if (FAILED(SetSecurityInfo(mutex.value,
                                   SE_KERNEL_OBJECT,
                                   DACL_SECURITY_INFORMATION,
                                   nullptr,
                                   nullptr,
                                   acl.value,
                                   nullptr))) {
            throw std::runtime_error("SetSecurityInfo failed");
        }
    }
    /**
     * @brief 启动微信
     *
     */
    std::unique_ptr<ProcessData> Run() {
        try {
            UnlimitSignle();
        } catch (...) {
            // ignore
        }
        STARTUPINFO si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        CreateProcess(NULL,
                      (LPTSTR)Path().c_str(),
                      NULL,
                      NULL,
                      FALSE,
                      DETACHED_PROCESS,
                      NULL,
                      NULL,
                      &si,
                      &pi);
        WaitForInputIdle(pi.hProcess, 5000);
        return std::make_unique<ProcessData>(pi.dwProcessId, pi.hProcess, pi.hThread);
    }

private:
    String path_;
};