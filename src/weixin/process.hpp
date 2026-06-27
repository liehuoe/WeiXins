#pragma once

#include <filesystem>
#include <windows.h>
#include <aclapi.h>
#include <cxxui/core/detail/reg_key.hpp>
#include "close_checker.hpp"

namespace weixin {

template <typename Derived>
class Process : public CloseChecker<Derived> {
    using Base = CloseChecker<Derived>;

public:
    /** @brief 获取微信可执行文件的路径
     *
     * @return std::filesystem::path 获取失败返回空路径
     */
    static std::filesystem::path GetAppPath() noexcept {
        cxxui::detail::RegKey key{HKEY_CURRENT_USER, L"SOFTWARE\\Tencent\\Weixin", KEY_READ};
        std::filesystem::path path = key.ReadString(L"InstallPath");
        if (!path.empty()) {
            path /= L"Weixin.exe";
        }
        return path;
    }
    /**
     * @brief 获取进程ID
     *
     * @return DWORD
     */
    DWORD GetPid() const noexcept { return pid_; }
    /**
     * @brief 启动微信
     * @return bool 是否启动成功
     */
    bool Start() noexcept {
        if (pid_ != 0) {
            return true;
        }
        UnlimitSignle();
        STARTUPINFOW si{};
        si.cb = sizeof(si);
        PROCESS_INFORMATION pi{};
        if (!CreateProcessW(nullptr,
                            const_cast<LPWSTR>(GetAppPath().c_str()),
                            nullptr,
                            nullptr,
                            FALSE,
                            DETACHED_PROCESS,
                            nullptr,
                            nullptr,
                            &si,
                            &pi)) {
            return false;
        }
        WaitForInputIdle(pi.hProcess, 5000);  // 等待进程启动完成
        CloseHandle(pi.hThread);
        pid_ = pi.dwProcessId;
        this->CheckClose(pi.hProcess);
        return true;
    }

private:
    DWORD pid_ = 0;

    static bool UnlimitSignle() noexcept {
        struct Mutex {
            HANDLE value = nullptr;
            Mutex(HANDLE v)
                : value(v) {}
            ~Mutex() {
                if (value) {
                    CloseHandle(value);
                }
            }
        };
        struct Sid {
            PSID value = nullptr;
            ~Sid() {
                if (value) {
                    FreeSid(value);
                }
            }
        };
        struct Acl {
            PACL value;
            static DWORD Size() { return sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE); };
            Acl() { value = (PACL)LocalAlloc(LPTR, Size()); }
            ~Acl() { LocalFree(value); }
        };

        Mutex mutex = CreateMutexW(nullptr, FALSE, L"XWeChat_App_Instance_Identity_Mutex_Name");
        if (!mutex.value) {
            return false;
        }
        Acl acl;
        if (!InitializeAcl(acl.value, acl.Size(), ACL_REVISION)) {
            return false;
        }
        Sid sid;
        SID_IDENTIFIER_AUTHORITY sid_auth = SECURITY_WORLD_SID_AUTHORITY;
        if (!AllocateAndInitializeSid(
                &sid_auth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &sid.value)) {
            return false;
        }
        if (!AddAccessDeniedAce(acl.value, ACL_REVISION, MUTEX_ALL_ACCESS, sid.value)) {
            return false;
        }
        if (FAILED(SetSecurityInfo(mutex.value,
                                   SE_KERNEL_OBJECT,
                                   DACL_SECURITY_INFORMATION,
                                   nullptr,
                                   nullptr,
                                   acl.value,
                                   nullptr))) {
            return false;
        }
        return true;
    }
};

}  // namespace weixin
