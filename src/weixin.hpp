#pragma once

#include "weixin/title.hpp"

class WeiXin : public weixin::Title<WeiXin> {
    using Base = weixin::Title<WeiXin>;

public:
    /**
     * @brief 创建微信进程，启动微信
     */
    static void Create(std::string bak_dir) {
        waiting_dirs_.emplace_back(std::move(bak_dir));
        if (waiting_dirs_.size() == 1) {
            CreateFirst();
        }
    }
    /**
     * @brief 检测是否所有微信都已经退出
     *
     * @return bool
     */
    static bool IsAllCloseed() noexcept { return Base::IsAllCloseed(); }
    /**
     * @brief 获取所有已经启动的微信
     *
     * @return const std::vector<WeiXin*>&
     */
    static const std::vector<WeiXin*>& GetAllObjs() noexcept { return Base::GetAllObjs(); }
    /**
     * @brief 获取正在排队的微信的备份目录列表
     *
     * @return const std::vector<std::string>&
     */
    static const std::vector<std::string>& GetWaitingDirs() noexcept { return waiting_dirs_; }
    using CloseHandler = void (*)();
    /**
     * @brief 设置微信退出时的处理函数
     *
     * @param handler
     */
    static void SetCloseHandler(CloseHandler handler) noexcept { close_handler_ = handler; }

protected:
    friend class weixin::CloseChecker<WeiXin>;
    /** 微信退出时触发 */
    void OnClose(HANDLE handle) {
        Base::OnClose(handle);
        CheckWaitingDirs();
        if (close_handler_) {
            close_handler_();
        }
        delete this;
    }

    friend class weixin::LoginChecker<WeiXin>;
    /** 微信登录成功时触发 */
    void OnLogin(HWND hwnd) {
        Base::OnLogin(hwnd);
        CheckWaitingDirs();
    }

private:
    WeiXin(std::string dir)
        : Base(std::move(dir)) {}

private:
    /** 需要登录的微信列表 */
    inline static std::vector<std::string> waiting_dirs_;
    /** 创建队列中的第一个微信 */
    static void CreateFirst() noexcept {
        namespace fs = std::filesystem;
        // 复制登录文件到微信
        weixin::CopyLoginFiles<true>(Config::GetInstance().GetUserDir() / waiting_dirs_[0]);
        // 删除微信缓存的头像目录
        fs::path head_imgs = weixin::GetHeadImgDir();
        if (fs::exists(head_imgs)) {
            fs::remove_all(head_imgs);
        }
        // 启动微信
        auto wx = new WeiXin{waiting_dirs_[0]};
        if (!wx->Start()) {
            delete wx;
            waiting_dirs_.clear();  // 清空后续登录
            MessageBoxW(nullptr, L"微信启动失败", L"微信多开助手", MB_OK | MB_ICONERROR);
            return;
        }
        wx->CheckLogin();
    }
    /** 检查是否还有需要启动的微信 */
    void CheckWaitingDirs() noexcept {
        if (waiting_dirs_.empty() || this->GetDir() != waiting_dirs_[0]) {
            return;
        }
        waiting_dirs_.erase(waiting_dirs_.begin());
        if (!waiting_dirs_.empty()) {
            CreateFirst();
        }
    }

private:
    inline static CloseHandler close_handler_ = nullptr;
};
