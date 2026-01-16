#pragma once

#include <any>
#include <array>
#include <cassert>
#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <utility>

#include "component_array.hpp"

namespace ecs {

// Handles component registrations and per-type storage.
class ComponentManager {
public:
    ComponentManager() = default;

    template <typename T>
    void RegisterComponent()
    {
        std::type_index typeName(typeid(T));
        assert(mComponentTypes.find(typeName) == mComponentTypes.end() && "Component registered twice");
        assert(mNextComponentType < MAX_COMPONENTS && "Too many component types");

        ComponentType type = mNextComponentType++;
        auto array = std::make_shared<ComponentArray<T>>();
        mComponentTypes[typeName] = type;
        mComponentArrays[type] = array;
        mDestroyCallbacks[type] = [array](Entity entity) {
            array->EntityDestroyed(entity);
        };
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
            if (mDestroyCallbacks[type]) {
                mDestroyCallbacks[type](entity);
            }
        }
    }

private:
    std::unordered_map<std::type_index, ComponentType> mComponentTypes{};
    std::array<std::any, MAX_COMPONENTS> mComponentArrays{};
    std::array<std::function<void(Entity)>, MAX_COMPONENTS> mDestroyCallbacks{};
    ComponentType mNextComponentType{};

    template <typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray()
    {
        ComponentType type = GetComponentType<T>();
        
        try {
            auto array = std::any_cast<std::shared_ptr<ComponentArray<T>>>(mComponentArrays[type]);
            assert(array && "Component array missing");
            return array;
        } catch (const std::bad_any_cast&) {
            assert(false && "Invalid component array type");
            return nullptr;
        }
    }
};

} // namespace ecs