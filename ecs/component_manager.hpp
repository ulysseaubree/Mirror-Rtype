#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "component_array.hpp"

namespace ecs {

// Handles component registrations and per-type storage.
class ComponentManager {
public:
    ComponentManager()
    {
        mDestroyCallbacks.fill(nullptr);
    }

    template <typename T>
    void RegisterComponent()
    {
        std::type_index typeName(typeid(T));
        assert(mComponentTypes.find(typeName) == mComponentTypes.end() && "Component registered twice");
        assert(mNextComponentType < MAX_COMPONENTS && "Too many component types");

        ComponentType type = mNextComponentType++;
        auto array = std::make_shared<ComponentArray<T>>();
        mComponentTypes[typeName] = type;
        mComponentArrays[type] = std::static_pointer_cast<void>(array);
        mDestroyCallbacks[type] = &ComponentManager::EntityDestroyedInvoker<T>;
    }

    template <typename T>
    ComponentType GetComponentType()
    {
        std::type_index typeName(typeid(T));
        auto it = mComponentTypes.find(typeName);
        assert(it != mComponentTypes.end() && "Component not registered");
        return it->second;
    }

    template <typename T>
    void AddComponent(Entity entity, T component)
    {
        GetComponentArray<T>()->InsertData(entity, std::move(component));
    }

    template <typename T>
    void RemoveComponent(Entity entity)
    {
        GetComponentArray<T>()->RemoveData(entity);
    }

    template <typename T>
    T& GetComponent(Entity entity)
    {
        return GetComponentArray<T>()->GetData(entity);
    }

    void EntityDestroyed(Entity entity)
    {
        for (ComponentType type = 0; type < mNextComponentType; ++type) {
            if (auto& storage = mComponentArrays[type]; storage && mDestroyCallbacks[type]) {
                mDestroyCallbacks[type](storage.get(), entity);
            }
        }
    }

private:
    std::unordered_map<std::type_index, ComponentType> mComponentTypes{};
    std::array<std::shared_ptr<void>, MAX_COMPONENTS> mComponentArrays{};
    std::array<void (*)(void*, Entity), MAX_COMPONENTS> mDestroyCallbacks{};
    ComponentType mNextComponentType{};

    template <typename T>
    static void EntityDestroyedInvoker(void* storage, Entity entity)
    {
        auto* typedArray = static_cast<ComponentArray<T>*>(storage);
        assert(typedArray && "Component storage missing");
        typedArray->EntityDestroyed(entity);
    }

    template <typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray()
    {
        ComponentType type = GetComponentType<T>();
        auto array = std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[type]);
        assert(array && "Component array missing");
        return array;
    }
};

} // namespace ecs

