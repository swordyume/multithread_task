#include "trade_system.h"

namespace guild {
    Transaction::Transaction(GuildRegistry &registry, std::mutex &tx_mutex)
        : registry_(registry), tx_mutex_(tx_mutex) {
    }

    std::optional<Attribute> Transaction::get(const std::string &key) {
        // TODO: 实现 read-your-writes（先查 write_set_，再查 registry_）
        // 步骤：
        //   1. 在 write_set_ 中查找 key，若存在则直接返回（read-your-writes）
        //   2. 否则从 registry_ 读取
        //      - 基础实现：调用 registry_.query(key)
        //      - 加分项：调用 registry_.query_versioned(key)，将版本号存入 read_set_
        auto it=write_set_.find(key);
        if (it != write_set_.end()) {return it->second;}
        return registry_.query(key);

    }

    void Transaction::put(const std::string &key, Attribute value) {
        // TODO: 记录到 write_set_

        write_set_.insert({key, value});

    }

    void Transaction::remove(const std::string &key) {
        // TODO: 在 write_set_ 中标记删除（存入 nullopt）
        write_set_[key] = std::nullopt;
    }

    bool Transaction::commit() {
        if (committed_ || rolled_back_) return false;
        std::unique_lock lock(tx_mutex_);
        // TODO: 实现原子提交
        // 基础实现：持全局提交锁，保证 write_set_ 原子写入
        // 同一时刻只有一个事务在提交，不会出现并发 lost update
        //
        // 加分项实现（通过 BonusShardLock 测试）：
        //   调用 registry_.atomic_write(write_set_, read_set_)
        //   - 若返回 true：提交成功，设置 committed_ = true，返回 true
        //   - 若返回 false：版本冲突，不修改 committed_，返回 false（调用方可重试）
        //   这种方式使用 shard 级别细粒度锁，不同 shard 的事务可以真正并行
        for (auto& it:write_set_)
        {
            if (it.second == std::nullopt) { registry_.dismiss(it.first);continue; }
            registry_.register_adventurer(it.first,it.second.value());
        }
        committed_ = true;

        return true;
    }

    void Transaction::rollback() {
        if (committed_ || rolled_back_) return;
        write_set_.clear();
        rolled_back_ = true;

    }

    Transaction::~Transaction() {
        if (!committed_ && !rolled_back_) {
            rollback();
        }
    }
} // namespace guild
