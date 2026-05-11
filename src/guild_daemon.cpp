#include "guild_daemon.h"

namespace guild {
    // ============================================================
    // ExpirySpirit
    // ============================================================

    ExpirySpirit::ExpirySpirit(GuildRegistry &registry, std::chrono::milliseconds patrol_interval)
        : registry_(registry), patrol_interval_(patrol_interval) {
    }

    void ExpirySpirit::start() {
        // TODO: 创建后台线程，执行 patrol_loop()
        if (thread_.joinable()) return;
        thread_=std::thread(&ExpirySpirit::patrol_loop,this);
        // thread_.detach();
    }

    void ExpirySpirit::stop() {
        // TODO: 停止守护精灵
        stop_flag_.store(true);
        cv_.notify_one();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    ExpirySpirit::~ExpirySpirit() {
        stop();
    }

    void ExpirySpirit::patrol_loop() {
        // TODO: 实现巡逻主循环
        while(true)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, patrol_interval_,[&](){return stop_flag_.load();});
            if (stop_flag_.load()) break;
            registry_.evict_all_expired();
        }

    }

    // ============================================================
    // StatsDaemon
    // ============================================================

    StatsDaemon::StatsDaemon(const GuildRegistry &registry, std::chrono::milliseconds report_interval)
        : registry_(registry), report_interval_(report_interval) {
    }

    void StatsDaemon::start() {
        // TODO: 实现启动
        if (thread_.joinable()) return;
        thread_=std::thread(&StatsDaemon::report_loop,this);
        // thread_.detach();
    }

    void StatsDaemon::stop() {
        // TODO:
        stop_flag_.store(true);
        cv_.notify_one();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    StatsDaemon::~StatsDaemon() {
        stop();
    }

    void StatsDaemon::report_loop() {
        // TODO: 实现统计报告主循环
        while(true)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait_for(lock, report_interval_,[&](){return stop_flag_.load();});
            if (stop_flag_.load()) break;
            registry_.stats().print();
        }
    }
} // namespace guild
