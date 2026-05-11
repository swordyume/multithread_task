#include "dispatch_pool.h"

namespace guild {
    DispatchPool::DispatchPool(size_t num_windows) {
        // TODO: 创建 num_windows 个 worker 线程，存入 workers_
        for (size_t i = 0; i < num_windows; ++i)
        {
            workers_.emplace_back(std::thread([this](){worker_loop();}));
        }
    }

    void DispatchPool::worker_loop() {
        // TODO: 循环从 board_ 取任务并执行，直到 board_ 关闭且为空时退出
        while (true)
        {
            auto task = board_.wait_and_take();
            if (task.has_value()) (*task)();
            else break;
        }
    }

    void DispatchPool::shutdown() {
        // TODO: 设置关闭标志，通知 board_ 停止，等待所有 worker 线程退出
        shutdown_=true;
        board_.close_board();
        for (auto& worker : workers_)
        {
            if (worker.joinable()) worker.join();
        }
    }

    DispatchPool::~DispatchPool() {
        shutdown();
    }
} // namespace guild
