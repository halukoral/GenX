#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <bitset>
#include <queue>
#include <algorithm>
#include <ranges>

namespace ECS
{

// Configuration
constexpr size_t MAX_COMPONENTS = 64;
constexpr size_t MAX_ENTITIES = 10000;

// Entity is just an ID
using Entity = uint32_t;
using ComponentType = std::size_t;
using Signature = std::bitset<MAX_COMPONENTS>;

// Component base class (optional, for RTTI)
struct Component
{
    virtual ~Component() = default;
};

// Component type counter
class ComponentTypeCounter
{
private:
    inline static ComponentType counter = 0;
public:
    template<typename T>
    static ComponentType GetTypeId()
	{
        static ComponentType typeID = counter++;
        return typeID;
    }
};

// Component Array Interface
class IComponentArray
{
public:
    virtual ~IComponentArray() = default;
    virtual void EntityDestroyed(Entity entity) = 0;
};

// Typed Component Array
template<typename T>
class ComponentArray : public IComponentArray
{
private:
    std::unordered_map<Entity, size_t> entityToIndex;
    std::unordered_map<size_t, Entity> indexToEntity;
    std::vector<T> components;
    size_t size = 0;

public:
    void Add(const Entity entity, const T& component)
	{
        if (entityToIndex.contains(entity))
        {
            // Update existing component
            components[entityToIndex[entity]] = component;
            return;
        }
        
        // Add new component
        entityToIndex[entity] = size;
        indexToEntity[size] = entity;
        components.push_back(component);
        size++;
    }
    
    void Remove(const Entity entity)
	{
        if (!entityToIndex.contains(entity)) return;
        
        size_t removedIndex = entityToIndex[entity];
        size_t lastIndex = size - 1;
        
        // Move last element to removed position
        components[removedIndex] = components[lastIndex];
        
        // Update maps
        const Entity lastEntity = indexToEntity[lastIndex];
        entityToIndex[lastEntity] = removedIndex;
        indexToEntity[removedIndex] = lastEntity;
        
        // Clean up
        entityToIndex.erase(entity);
        indexToEntity.erase(lastIndex);
        components.pop_back();
        size--;
    }
    
    T& Get(const Entity entity)
	{
        return components[entityToIndex[entity]];
    }
    
    bool Has(const Entity entity) const
	{
        return entityToIndex.contains(entity);
    }
    
    void EntityDestroyed(const Entity entity) override
	{
        Remove(entity);
    }
};

// System base class
class System
{
public:
    std::vector<Entity> Entities;
    
    virtual void Update(float dt) = 0;
    virtual ~System() = default;
};

// Component Manager
class ComponentManager
{
private:
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays;
    
    template<typename T>
    std::shared_ptr<ComponentArray<T>> GetComponentArray()
	{
        const auto typeIndex = std::type_index(typeid(T));
        if (!componentArrays.contains(typeIndex))
        {
            componentArrays[typeIndex] = std::make_shared<ComponentArray<T>>();
        }
        return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeIndex]);
    }
    
public:
    template<typename T>
    void AddComponent(Entity entity, const T& component)
	{
        GetComponentArray<T>()->Add(entity, component);
    }
    
    template<typename T>
    void RemoveComponent(Entity entity)
	{
        GetComponentArray<T>()->Remove(entity);
    }
    
    template<typename T>
    T& GetComponent(Entity entity)
	{
        return GetComponentArray<T>()->Get(entity);
    }
    
    template<typename T>
    bool HasComponent(Entity entity)
	{
        return GetComponentArray<T>()->Has(entity);
    }
    
    void EntityDestroyed(Entity entity)
	{
        for (const auto& array : componentArrays | std::views::values)
        {
            array->EntityDestroyed(entity);
        }
    }
};

// Entity Manager
class EntityManager
{
private:
    std::queue<Entity> availableEntities;
    std::unordered_map<Entity, Signature> signatures;
    uint32_t entityCount = 0;
    
public:
    EntityManager()
	{
        for (Entity entity = 0; entity < MAX_ENTITIES; ++entity) {
            availableEntities.push(entity);
        }
    }
    
    Entity CreateEntity()
	{
        const Entity id = availableEntities.front();
        availableEntities.pop();
        entityCount++;
        return id;
    }
    
    void DestroyEntity(const Entity entity)
	{
        signatures[entity].reset();
        availableEntities.push(entity);
        entityCount--;
    }
    
    void SetSignature(const Entity entity, const Signature signature)
	{
        signatures[entity] = signature;
    }
    
    Signature GetSignature(const Entity entity)
	{
        return signatures[entity];
    }
};

// System Manager
class SystemManager
{
private:
    std::unordered_map<std::type_index, std::shared_ptr<System>> systems;
    std::unordered_map<std::type_index, Signature> systemSignatures;
    
public:
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
	{
        auto system = std::make_shared<T>();
        systems[std::type_index(typeid(T))] = system;
        return system;
    }
    
    template<typename T>
    void SetSignature(const Signature signature)
	{
        systemSignatures[std::type_index(typeid(T))] = signature;
    }
    
    void EntityDestroyed(const Entity entity)
	{
        for (const auto& system : systems | std::views::values)
        {
            auto& entities = system->Entities;
            std::erase(entities, entity);
        }
    }
    
    void EntitySignatureChanged(const Entity entity, const Signature entitySignature)
	{
        for (auto& [type, system] : systems)
        {
            auto& systemSignature = systemSignatures[type];
            auto& systemEntities = system->Entities;
            
            // Check if entity signature matches system signature
            if ((entitySignature & systemSignature) == systemSignature)
            {
                // Add entity if not already in system
                if (std::ranges::find(systemEntities, entity) == systemEntities.end())
                {
                    systemEntities.push_back(entity);
                }
            }
        	else
        	{
                // Remove entity if in system
                std::erase(systemEntities, entity);
            }
        }
    }
    
    void Update(const float dt)
	{
        for (const auto& system : systems | std::views::values)
        {
            system->Update(dt);
        }
    }
};

// World - Main ECS interface
class World
{
private:
    std::unique_ptr<ComponentManager> componentManager;
    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<SystemManager> systemManager;
    
public:
    World()
	{
        componentManager = std::make_unique<ComponentManager>();
        entityManager = std::make_unique<EntityManager>();
        systemManager = std::make_unique<SystemManager>();
    }
    
    // Entity methods
    Entity CreateEntity() const
	{
        return entityManager->CreateEntity();
    }
    
    void DestroyEntity(const Entity entity) const
	{
        entityManager->DestroyEntity(entity);
        componentManager->EntityDestroyed(entity);
        systemManager->EntityDestroyed(entity);
    }
    
    // Component methods
    template<typename T>
    void AddComponent(const Entity entity, const T& component)
	{
        componentManager->AddComponent<T>(entity, component);
        
        auto signature = entityManager->GetSignature(entity);
        signature.set(ComponentTypeCounter::GetTypeId<T>(), true);
        entityManager->SetSignature(entity, signature);
        
        systemManager->EntitySignatureChanged(entity, signature);
    }
    
    template<typename T>
    void RemoveComponent(const Entity entity) const
	{
        componentManager->RemoveComponent<T>(entity);
        
        auto signature = entityManager->GetSignature(entity);
        signature.set(ComponentTypeCounter::GetTypeId<T>(), false);
        entityManager->SetSignature(entity, signature);
        
        systemManager->EntitySignatureChanged(entity, signature);
    }
    
    template<typename T>
    T& GetComponent(const Entity entity)
	{
        return componentManager->GetComponent<T>(entity);
    }
    
    template<typename T>
	[[nodiscard]] bool HasComponent(const Entity entity) const
	{
        return componentManager->HasComponent<T>(entity);
    }
    
    // System methods
    template<typename T>
    std::shared_ptr<T> RegisterSystem()
	{
        return systemManager->RegisterSystem<T>();
    }
    
    template<typename T>
    void SetSystemSignature(const Signature signature) const
	{
        systemManager->SetSignature<T>(signature);
    }
    
    void Update(const float dt) const
	{
        systemManager->Update(dt);
    }
};

} // namespace ECS