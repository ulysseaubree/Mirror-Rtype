#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ecs {

using Entity = std::uint32_t;
inline constexpr Entity MAX_ENTITIES = 5000;

using ComponentType = std::uint8_t;
inline constexpr ComponentType MAX_COMPONENTS = 64;

using Signature = std::bitset<MAX_COMPONENTS>;

// Manages entity identifiers and their signatures.
class EntityManager {
public:
    EntityManager()
    {
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
            mAvailableEntities.push(entity);
        }
    }

    Entity CreateEntity()
    {
        assert(mLivingEntityCount < MAX_ENTITIES && "Too many entities");
        Entity id = mAvailableEntities.front();
        mAvailableEntities.pop();
        ++mLivingEntityCount;
        return id;
    }

    void DestroyEntity(Entity entity)
    {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        mSignatures[entity].reset();
        mAvailableEntities.push(entity);
        --mLivingEntityCount;
    }

    void SetSignature(Entity entity, Signature signature)
    {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        mSignatures[entity] = signature;
    }

    Signature GetSignature(Entity entity) const
    {
        assert(entity < MAX_ENTITIES && "Entity out of range");
        return mSignatures[entity];
    }

private:
    std::queue<Entity> mAvailableEntities{};
    std::array<Signature, MAX_ENTITIES> mSignatures{};
    std::uint32_t mLivingEntityCount{};
};

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

// Base system: holds signature and matching entities.
class System {
public:
    virtual ~System() = default;
    Signature signature{};
    std::vector<Entity> entities{};
};

// Tracks systems and syncs membership with signatures.
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

// High-level facade combining managers.
class Coordinator {
public:
    void Init()
    {
        mEntityManager = std::make_unique<EntityManager>();
        mComponentManager = std::make_unique<ComponentManager>();
        mSystemManager = std::make_unique<SystemManager>();
    }

    Entity CreateEntity()
    {
        return mEntityManager->CreateEntity();
    }

    void DestroyEntity(Entity entity)
    {
        mComponentManager->EntityDestroyed(entity);
        mSystemManager->EntityDestroyed(entity);
        mEntityManager->DestroyEntity(entity);
    }

    template <typename T>
    void RegisterComponent()
    {
        mComponentManager->RegisterComponent<T>();
    }

    template <typename T>
    void AddComponent(Entity entity, T component)
    {
        mComponentManager->AddComponent<T>(entity, std::move(component));

        Signature signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentManager->GetComponentType<T>(), true);
        mEntityManager->SetSignature(entity, signature);
        mSystemManager->EntitySignatureChanged(entity, signature);
    }

    template <typename T>
    void RemoveComponent(Entity entity)
    {
        mComponentManager->RemoveComponent<T>(entity);

        Signature signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentManager->GetComponentType<T>(), false);
        mEntityManager->SetSignature(entity, signature);
        mSystemManager->EntitySignatureChanged(entity, signature);
    }

    template <typename T>
    T& GetComponent(Entity entity)
    {
        return mComponentManager->GetComponent<T>(entity);
    }

    template <typename T>
    ComponentType GetComponentType()
    {
        return mComponentManager->GetComponentType<T>();
    }

    template <typename T, typename... Args>
    std::shared_ptr<T> RegisterSystem(Args&&... args)
    {
        return mSystemManager->RegisterSystem<T>(std::forward<Args>(args)...);
    }

    template <typename T>
    void SetSystemSignature(Signature signature)
    {
        mSystemManager->SetSignature<T>(signature);
    }

private:
    std::unique_ptr<EntityManager> mEntityManager{};
    std::unique_ptr<ComponentManager> mComponentManager{};
    std::unique_ptr<SystemManager> mSystemManager{};
};

inline Coordinator gCoordinator{};

// Example usage --------------------------------------------------------------
struct Position {
    float x;
    float y;
};

struct Velocity {
    float vx;
    float vy;
};

class MovementSystem : public System {
public:
    void Update(float dt)
    {
        for (Entity entity : entities) {
            auto& position = gCoordinator.GetComponent<Position>(entity);
            const auto& velocity = gCoordinator.GetComponent<Velocity>(entity);
            position.x += velocity.vx * dt;
            position.y += velocity.vy * dt;
        }
    }
};

inline void ExampleECS()
{
    gCoordinator.Init();
    gCoordinator.RegisterComponent<Position>();
    gCoordinator.RegisterComponent<Velocity>();

    auto movement = gCoordinator.RegisterSystem<MovementSystem>();
    Signature signature;
    signature.set(gCoordinator.GetComponentType<Position>());
    signature.set(gCoordinator.GetComponentType<Velocity>());
    gCoordinator.SetSystemSignature<MovementSystem>(signature);

    Entity e = gCoordinator.CreateEntity();
    gCoordinator.AddComponent(e, Position{0.0f, 0.0f});
    gCoordinator.AddComponent(e, Velocity{1.0f, 0.0f});

    movement->Update(1.0f);
}

} // namespace ecs
