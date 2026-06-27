#pragma once

#include <vector>
#include <windows.h>

namespace weixin {

template <typename Derived>
class CloseChecker {
protected:
    ~CloseChecker() {
        if (handle_) {
            CloseHandle(handle_);
        }
        if (idx_ != -1) {
            objs_[idx_] = nullptr;
        }
    }

public:
    /**
     * @brief 获取进程句柄
     *
     * @return HANDLE
     */
    HANDLE GetHandle() const noexcept { return handle_; }
    /**
     * @brief 获取微信窗口的序号
     *
     * @return int
     */
    int GetIndex() const noexcept { return idx_; }

protected:
    void CheckClose(HANDLE handle) {
        handle_ = handle;
        idx_ = static_cast<int>(handles_.size());
        handles_.emplace_back(handle);
        objs_.emplace_back(static_cast<Derived*>(this));
    }
    /** 微信退出时触发 */
    void OnClose(HANDLE) {}
    /**
     * @brief 判断是否所有微信都已经退出
     *
     * @return bool
     */
    static bool IsAllCloseed() {
        while (true) {
            if (handles_.empty()) {
                return true;
            }
            DWORD rc = MsgWaitForMultipleObjects(
                static_cast<DWORD>(handles_.size()), handles_.data(), FALSE, INFINITE, QS_ALLINPUT);
            if (rc < WAIT_OBJECT_0 || rc >= WAIT_OBJECT_0 + handles_.size()) {
                return false;
            }
            Remove(rc - WAIT_OBJECT_0);
        }
        return false;
    }
    static const std::vector<Derived*>& GetAllObjs() noexcept { return objs_; }

private:
    int idx_ = -1;
    HANDLE handle_ = nullptr;
    inline static std::vector<Derived*> objs_;
    inline static std::vector<HANDLE> handles_;
    static void Remove(size_t idx) {
        if (objs_[idx]) {
            static_cast<Derived*>(objs_[idx])->OnClose(handles_[idx]);
        }
        handles_.erase(handles_.begin() + idx);
        objs_.erase(objs_.begin() + idx);
        for (size_t i = idx; i < handles_.size(); i++) {
            --objs_[i]->idx_;
        }
    }
};

}  // namespace weixin