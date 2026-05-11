#include "expedition.h"

namespace guild {
    // ============================================================
    // ExpeditionClient
    // ============================================================

    std::future<std::optional<Attribute> > ExpeditionClient::async_query(const std::string &key) {
        // TODO: 实现异步查询
        std::future<std::optional<Attribute>> future=pool_.dispatch([this,key]()
            -> std::optional<Attribute>
            {return registry_.query(key);});
        return future;
    }

    std::future<void> ExpeditionClient::async_register(const std::string &key, Attribute value,
                                                       std::chrono::seconds ttl) {
        // TODO: 实现异步注册
        std::future<void> future=pool_.dispatch([this,key,value,ttl]()
            -> void
            {registry_.register_adventurer(key,value,ttl);});

        return future;
    }

    std::future<bool> ExpeditionClient::async_dismiss(const std::string &key) {
        // TODO: 实现异步删除
        std::future<bool> future=pool_.dispatch([this,key]()
            -> bool
            {return registry_.dismiss(key);});

        return future;
    }

    void ExpeditionClient::async_query_with_callback(
        const std::string &key,
        std::function<void(std::optional<Attribute>)> callback) {
        // TODO: 实现带回调的异步查询（加分项）
        (void) key;
        (void) callback;
    }

    // ============================================================
    // BatchDispatcher
    // ============================================================

    std::vector<QuestResult> BatchDispatcher::launch(const std::vector<Quest> &quests) {
        // TODO: 实现批量并行执行
        // std::vector<QuestResult> results;
        // QuestResult result;
        // for (auto quest:quests)
        // {
        //     if (quest.type==QuestType::kQuery)
        //     {
        //         std::future<std::optional<Attribute>> future=pool_.dispatch([this,quest]()
        //             -> std::optional<Attribute>
        //             {return registry_.query(quest.key);});
        //         if (future.get().has_value()) result=QuestResult::ok(future.get().value());
        //         else result=QuestResult::not_found();
        //         results.push_back(result);
        //     }
        //     else if (quest.type==QuestType::kRegister)
        //     {
        //         std::future<void> future=pool_.dispatch([this,quest]()
        //             -> void
        //             {registry_.register_adventurer(quest.key,quest.value.value(),quest.ttl);});
        //         result=QuestResult::ok();
        //         results.push_back(result);
        //     }
        //     else if (quest.type==QuestType::kRemove)
        //     {
        //         std::future<bool> future=pool_.dispatch([this,quest]()
        //             -> bool
        //             {return registry_.dismiss(quest.key);});
        //         result=QuestResult::ok();
        //         results.push_back(result);
        //     }
        // }
        // return results;
        std::vector<std::future<QuestResult>> futures;

        for (const auto& quest : quests) {
            futures.push_back(
                pool_.dispatch([this, quest]() -> QuestResult {
                    switch (quest.type) {
                        case QuestType::kQuery: {
                            auto opt = registry_.query(quest.key);
                            if (opt.has_value())
                                return QuestResult::ok(std::move(*opt));
                            else
                                return QuestResult::not_found();
                        }
                        case QuestType::kRegister: {
                            registry_.register_adventurer(quest.key,
                                                          quest.value.value(),
                                                          quest.ttl);
                            return QuestResult::ok();
                        }
                        case QuestType::kRemove: {
                            bool ok = registry_.dismiss(quest.key);
                            return ok ? QuestResult::ok() : QuestResult::not_found();
                        }
                        default:
                        return {Status::kError, std::nullopt};
                }
            })
        );
        }

        std::vector<QuestResult> results;
        for (auto& f : futures) {
            results.push_back(f.get());   // 此时才等待所有结果
        }
        return results;
    }
} // namespace guild
