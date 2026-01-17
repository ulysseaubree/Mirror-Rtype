#pragma once

#include <algorithm>
#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "types.hpp"

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
        mSystemEntities[typeName] = &system->entities;
        return system;
    }

    template <typename T>
    void SetSignature(Signature signature)
    {
        std::type_index typeName(typeid(T));
        auto it = mSystems.find(typeName);
        assert(it != mSystems.end() && "System not registered");

        mSignatures[typeName] = signature;
    }

    void EntityDestroyed(Entity entity)
    {
        for (auto& [typeName, _] : mSystems) {
            auto it = mSystemEntities.find(typeName);
            if (it != mSystemEntities.end() && it->second) {
                RemoveEntity(*it->second, entity);
            }
        }
    }

    void EntitySignatureChanged(Entity entity, Signature entitySignature)
    {
        for (auto& [typeName, _] : mSystems) {
            auto sigIt = mSignatures.find(typeName);
            auto entIt = mSystemEntities.find(typeName);
            
            if (sigIt != mSignatures.end() && entIt != mSystemEntities.end() && entIt->second) {
                const Signature& required = sigIt->second;
                bool matches = (entitySignature & required) == required;
                
                if (matches) {
                    if (!Contains(*entIt->second, entity)) {
                        entIt->second->push_back(entity);
                    }
                } else {
                    RemoveEntity(*entIt->second, entity);
                }
            }
        }
    }

    template <typename T>
    std::shared_ptr<T> GetSystem()
    {
        std::type_index typeName(typeid(T));
        auto it = mSystems.find(typeName);
        assert(it != mSystems.end() && "System not registered");
        return std::static_pointer_cast<T>(it->second);
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
    std::unordered_map<std::type_index, std::shared_ptr<void>> mSystems{};
    std::unordered_map<std::type_index, std::vector<Entity>*> mSystemEntities{};
};

} // namespace ecs