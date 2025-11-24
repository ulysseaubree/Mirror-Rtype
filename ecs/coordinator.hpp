#pragma once

#include <memory>
#include <utility>

#include "component_manager.hpp"
#include "entity_manager.hpp"
#include "system_manager.hpp"

namespace ecs {

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

} // namespace ecs

