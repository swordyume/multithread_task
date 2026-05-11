#include "guild_registry.h"
#include <iostream>
#include <mutex>
#include <algorithm>
#include <map>

namespace guild {
    // ============================================================
    // StatsOrb
    // ============================================================

    void StatsOrb::print() const {
        std::cout << "=== 公会统计水晶球 ===\n"
                << "  查询次数: " << query_count.load() << "\n"
                << "  命中次数: " << hit_count.load() << "\n"
                << "  未命中:   " << miss_count.load() << "\n"
                << "  注册次数: " << register_count.load() << "\n"
                << "  删除次数: " << remove_count.load() << "\n"
                << "  过期清理: " << expired_count.load() << "\n"
                << "=====================\n";
    }

    // ============================================================
    // Shard
    // ============================================================

    std::optional<Attribute> Shard::get(const std::string &key) const {
        // TODO: 实现查询
        std::shared_lock lock(mutex_);
        auto it=data_.find(key);
        if (it != data_.end())
        {
            auto result = it->second;
            if (!result.is_expired())
            {
                Attribute value=result.value;
                return value;
            }
        }
        return std::nullopt;
    }

    void Shard::put(const std::string &key, Attribute value, std::chrono::seconds ttl) {
        // TODO: 实现写入
        std::unique_lock lock(mutex_);
        auto it=data_.find(key);
        if (it != data_.end())
        {
            it->second.value = value;
            it->second.version++;
            if (ttl == 0s) it->second.expire_at=std::chrono::steady_clock::time_point::max();
            else it->second.expire_at=std::chrono::steady_clock::now()+ttl;

        }
        else
        {
            Record record;
            record.value=value;
            record.version = 1;
            if (ttl != 0s) record.expire_at=std::chrono::steady_clock::now()+ttl;
            else record.expire_at=std::chrono::steady_clock::time_point::max();

            data_.emplace(key, std::move(record));
        }
    }

    bool Shard::remove(const std::string &key) {
        // TODO: 实现删除
        std::unique_lock lock(mutex_);
        auto it=data_.find(key);
        if (it != data_.end())
        {
            data_.erase(it);
            return true;
        }
        return false;
    }

    size_t Shard::evict_expired() {
        // TODO: 实现过期清理
        std::unique_lock lock(mutex_);
        size_t count{0};
        auto it = data_.begin();
        while (it != data_.end()) {
            if (it->second.is_expired()) {
                it = data_.erase(it);
                ++count;
            } else {
                ++it;
            }
        }
        return count;
    }

    size_t Shard::size() const {
        std::shared_lock lock(mutex_);
        return data_.size();
    }

    std::vector<std::string> Shard::keys() const {
        std::shared_lock lock(mutex_);
        std::vector<std::string> result;
        result.reserve(data_.size());
        for (const auto &[k, _]: data_) {
            result.push_back(k);
        }
        return result;
    }

    // ============================================================
    // GuildRegistry
    // ============================================================

    GuildRegistry::GuildRegistry(size_t shard_count) {
        shards_.reserve(shard_count);
        for (size_t i = 0; i < shard_count; ++i) {
            shards_.push_back(std::make_unique<Shard>());
        }
    }

    Shard &GuildRegistry::get_shard(const std::string &key) {
        // TODO: 实现分片路由
        size_t idx=std::hash<std::string>{}(key)%shards_.size();
        return *shards_[idx]; // 删除此行，替换为正确实现
    }

    const Shard &GuildRegistry::get_shard(const std::string &key) const {
        // TODO: 与上面相同，const 版本
        size_t idx=std::hash<std::string>{}(key)%shards_.size();
        return *shards_[idx];
    }

    std::optional<Attribute> GuildRegistry::query(const std::string &key) {
        // TODO: 实现查询，并更新统计计数器
        stats_.query_count.fetch_add(1);
        Shard &shard = get_shard(key);
        auto value=shard.get(key);
        if (value.has_value())
            stats_.hit_count.fetch_add(1);
        else
            stats_.miss_count.fetch_add(1);
        // auto it=shard.data_.find(key);
        // if (it != shard.data_.end())
        // {
        //     Attribute value=it->second.value;
        //     return value;
        // }
        // return std::nullopt;
        return value;
    }

    void GuildRegistry::register_adventurer(const std::string &key, Attribute value,
                                            std::chrono::seconds ttl) {
        // TODO: 实现注册，并更新统计计数器
        stats_.register_count.fetch_add(1);
        Shard &shard = get_shard(key);
        shard.put(key, value,ttl);
    }

    bool GuildRegistry::dismiss(const std::string &key) {
        // TODO: 实现删除，并更新统计计数器
        stats_.remove_count.fetch_add(1);
        Shard &shard = get_shard(key);
        return shard.remove(key);
    }

    size_t GuildRegistry::evict_all_expired() {
        size_t total = 0;
        for (auto &shard: shards_) {
            total += shard->evict_expired();
        }
        stats_.expired_count.fetch_add(total);
        return total;
    }

    size_t GuildRegistry::total_size() const {
        size_t total = 0;
        for (const auto &shard: shards_) {
            total += shard->size();
        }
        return total;
    }

    std::vector<std::string> GuildRegistry::all_keys() const {
        std::vector<std::string> result;
        for (const auto &shard: shards_) {
            auto keys = shard->keys();
            result.insert(result.end(), keys.begin(), keys.end());
        }
        return result;
    }

    // ============================================================
    // 加分项2：细粒度锁 + OCC（可选实现）
    // ============================================================

    std::pair<std::optional<Attribute>, uint64_t> Shard::get_versioned(
        const std::string &key) const {
        // TODO [加分项]: 与 get() 类似，但同时返回版本号
        // 提示：加共享读锁，查找 key，返回 {value, version}；不存在时返回 {nullopt, 0}
        (void) key;
        return {std::nullopt, 0};
    }

    void Shard::put_unlocked(const std::string &key, Attribute value) {
        // TODO [加分项]: 不加锁的写入（调用方已持有该 Shard 的 unique_lock）
        // 重要：只更新 value 字段，然后将 version 递增。
        // 提示：data_.emplace(key, Record) 在 key 不存在时插入，存在时返回已有迭代器
        (void) key;
        (void) value;
    }

    bool Shard::remove_unlocked(const std::string &key) {
        // TODO [加分项]: 不加锁的删除（调用方已持有该 Shard 的 unique_lock）
        (void) key;
        return false;
    }

    std::pair<std::optional<Attribute>, uint64_t> GuildRegistry::query_versioned(
        const std::string &key) {
        // TODO [加分项]: 带版本号的查询，同时更新统计计数器
        // 提示：调用 get_shard(key).get_versioned(key)，根据结果更新 hit/miss 计数
        stats_.query_count++;
        (void) key;
        stats_.miss_count++;
        return {std::nullopt, 0};
    }

    bool GuildRegistry::atomic_write(
        const std::map<std::string, std::optional<Attribute>> &write_set,
        const std::map<std::string, uint64_t> &read_set) {
        // TODO [加分项]: 带 OCC 校验的原子写入
        // 在 atomic_write 的基础上，持锁后先校验版本号：
        //   1. 收集 write_set + read_set 中所有 key 的 shard 索引，去重排序
        //   2. 按序加锁
        //   3. 遍历 read_set，检查每个 key 的当前版本是否等于记录的版本
        //      - 不等则说明有冲突，直接 return false（锁会在 unique_lock 析构时自动释放）
        //   4. 版本校验通过，应用 write_set 中的写操作
        //   5. return true
        (void) write_set;
        (void) read_set;
        return true;
    }
} // namespace guild
