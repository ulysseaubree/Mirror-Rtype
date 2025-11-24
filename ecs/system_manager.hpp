#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "system.hpp"

namespace ecs {

// Tracks systems and syncs membership with entity signatures.
class SystemManager {
public:
    template <typename T, typename... Args>
    std::shared_ptr<T> RegisterSystem(Args&&... args)
    {
        std::type_index typeName(typeid(T));
        assert(mSystems.find(typeName) == mSystems.end() && "System registered twice");

        auto system = std::make_shared<T>(std::forward<Args>(args)...);
        mSystems[typeName] = system;
        return system;
    }

    template <typename T>
    void SetSignature(Signature signature)
    {
        std::type_index typeName(typeid(T));
        auto it = mSystems.find(typeName);
        assert(it != mSystems.end() && "System not registered");

        mSignatures[typeName] = signature;
        it->second->signature = signature;
    }

    void EntityDestroyed(Entity entity)
    {
        for (auto& [_, system] : mSystems) {
            RemoveEntity(system->entities, entity);
        }
    }

    void EntitySignatureChanged(Entity entity, Signature entitySignature)
    {
        for (auto& [type, system] : mSystems) {
            const Signature& required = mSignatures[type];
            bool matches = (entitySignature & required) == required;
            if (matches) {
                if (!Contains(system->entities, entity)) {
                    system->entities.push_back(entity);
                }
            } else {
                RemoveEntity(system->entities, entity);
            }
        }
    }

private:
    static void RemoveEntity(std::vector<Entity>& entities, Entity entity)
    {
        entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
    }

    static bool Contains(const std::vector<Entity>& entities, Entity entity)
    {
        return std::find(entities.begin(), entities.end(), entity) != entities.end();
    }

    std::unordered_map<std::type_index, Signature> mSignatures{};
    std::unordered_map<std::type_index, std::shared_ptr<System>> mSystems{};
};

} // namespace ecs
