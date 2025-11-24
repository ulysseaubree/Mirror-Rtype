#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <unordered_map>
#include <utility>

#include "types.hpp"

namespace ecs {

// Dense storage for a single component type with swap-delete removal.
template <typename T>
class ComponentArray {
public:
    void InsertData(Entity entity, T component)
    {
        auto [it, inserted] = mEntityToIndex.emplace(entity, mSize);
        assert(inserted && "Component added twice");

        std::size_t newIndex = it->second;
        mIndexToEntity[newIndex] = entity;
        mComponentArray[newIndex] = std::move(component);
        ++mSize;
    }

    void RemoveData(Entity entity)
    {
        auto it = mEntityToIndex.find(entity);
        assert(it != mEntityToIndex.end() && "Removing non-existent component");
        assert(mSize > 0 && "Component storage empty");

        std::size_t removedIndex = it->second;
        std::size_t lastIndex = mSize - 1;

        if (removedIndex != lastIndex) {
            Entity movedEntity = mIndexToEntity[lastIndex];
            mComponentArray[removedIndex] = std::move(mComponentArray[lastIndex]);
            mIndexToEntity[removedIndex] = movedEntity;
            mEntityToIndex[movedEntity] = removedIndex;
        }

        mEntityToIndex.erase(it);
        mIndexToEntity[lastIndex] = Entity{};
        --mSize;
    }

    T& GetData(Entity entity)
    {
        auto it = mEntityToIndex.find(entity);
        assert(it != mEntityToIndex.end() && "Retrieving non-existent component");
        return mComponentArray[it->second];
    }

    void EntityDestroyed(Entity entity)
    {
        auto it = mEntityToIndex.find(entity);
        if (it != mEntityToIndex.end()) {
            RemoveData(entity);
        }
    }

private:
    std::array<T, MAX_ENTITIES> mComponentArray{};
    std::array<Entity, MAX_ENTITIES> mIndexToEntity{};
    std::unordered_map<Entity, std::size_t> mEntityToIndex{};
    std::size_t mSize{};
};

} // namespace ecs

