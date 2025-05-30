#pragma once
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <bitset>
#include <queue>
#include <algorithm>

namespace ECS {

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
    static ComponentType getTypeID()
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
    virtual void entityDestroyed(Entity entity) = 0;
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
    void add(Entity entity, T component)
	{
        if (entityToIndex.find(entity) != entityToIndex.end())
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
    
    void remove(Entity entity)
	{
        if (entityToIndex.find(entity) == entityToIndex.end()) return;
        
        size_t removedIndex = entityToIndex[entity];
        size_t lastIndex = size - 1;
        
        // Move last element to removed position
        components[removedIndex] = components[lastIndex];
        
        // Update maps
        Entity lastEntity = indexToEntity[lastIndex];
        entityToIndex[lastEntity] = removedIndex;
        indexToEntity[removedIndex] = lastEntity;
        
        // Clean up
        entityToIndex.erase(entity);
        indexToEntity.erase(lastIndex);
        components.pop_back();
        size--;
    }
    
    T& get(Entity entity)
	{
        return components[entityToIndex[entity]];
    }
    
    bool has(Entity entity)
	{
        return entityToIndex.find(entity) != entityToIndex.end();
    }
    
    void entityDestroyed(Entity entity) override
	{
        remove(entity);
    }
};

// System base class
class System
{
public:
    std::vector<Entity> entities;
    
    virtual void update(float dt) = 0;
    virtual ~System() = default;
};

// Component Manager
class ComponentManager
{
private:
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays;
    
    template<typename T>
    std::shared_ptr<ComponentArray<T>> getComponentArray()
	{
        auto typeIndex = std::type_index(typeid(T));
        if (componentArrays.find(typeIndex) == componentArrays.end())
        {
            componentArrays[typeIndex] = std::make_shared<ComponentArray<T>>();
        }
        return std::static_pointer_cast<ComponentArray<T>>(componentArrays[typeIndex]);
    }
    
public:
    template<typename T>
    void addComponent(Entity entity, T component)
	{
        getComponentArray<T>()->add(entity, component);
    }
    
    template<typename T>
    void removeComponent(Entity entity)
	{
        getComponentArray<T>()->remove(entity);
    }
    
    template<typename T>
    T& getComponent(Entity entity)
	{
        return getComponentArray<T>()->get(entity);
    }
    
    template<typename T>
    bool hasComponent(Entity entity)
	{
        return getComponentArray<T>()->has(entity);
    }
    
    void entityDestroyed(Entity entity)
	{
        for (auto& [type, array] : componentArrays)
        {
            array->entityDestroyed(entity);
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
    
    Entity createEntity()
	{
        Entity id = availableEntities.front();
        availableEntities.pop();
        entityCount++;
        return id;
    }
    
    void destroyEntity(Entity entity)
	{
        signatures[entity].reset();
        availableEntities.push(entity);
        entityCount--;
    }
    
    void setSignature(Entity entity, Signature signature)
	{
        signatures[entity] = signature;
    }
    
    Signature getSignature(Entity entity)
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
    std::shared_ptr<T> registerSystem()
	{
        auto system = std::make_shared<T>();
        systems[std::type_index(typeid(T))] = system;
        return system;
    }
    
    template<typename T>
    void setSignature(Signature signature)
	{
        systemSignatures[std::type_index(typeid(T))] = signature;
    }
    
    void entityDestroyed(Entity entity)
	{
        for (auto& [type, system] : systems)
        {
            auto& entities = system->entities;
            entities.erase(std::remove(entities.begin(), entities.end(), entity), entities.end());
        }
    }
    
    void entitySignatureChanged(Entity entity, Signature entitySignature)
	{
        for (auto& [type, system] : systems)
        {
            auto& systemSignature = systemSignatures[type];
            auto& systemEntities = system->entities;
            
            // Check if entity signature matches system signature
            if ((entitySignature & systemSignature) == systemSignature)
            {
                // Add entity if not already in system
                if (std::find(systemEntities.begin(), systemEntities.end(), entity) == systemEntities.end())
                {
                    systemEntities.push_back(entity);
                }
            }
        	else
        	{
                // Remove entity if in system
                systemEntities.erase(std::remove(systemEntities.begin(), systemEntities.end(), entity), systemEntities.end());
            }
        }
    }
    
    void update(float dt)
	{
        for (auto& [type, system] : systems)
        {
            system->update(dt);
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
    World() {
        componentManager = std::make_unique<ComponentManager>();
        entityManager = std::make_unique<EntityManager>();
        systemManager = std::make_unique<SystemManager>();
    }
    
    // Entity methods
    Entity createEntity()
	{
        return entityManager->createEntity();
    }
    
    void destroyEntity(Entity entity)
	{
        entityManager->destroyEntity(entity);
        componentManager->entityDestroyed(entity);
        systemManager->entityDestroyed(entity);
    }
    
    // Component methods
    template<typename T>
    void addComponent(Entity entity, T component)
	{
        componentManager->addComponent<T>(entity, component);
        
        auto signature = entityManager->getSignature(entity);
        signature.set(ComponentTypeCounter::getTypeID<T>(), true);
        entityManager->setSignature(entity, signature);
        
        systemManager->entitySignatureChanged(entity, signature);
    }
    
    template<typename T>
    void removeComponent(Entity entity)
	{
        componentManager->removeComponent<T>(entity);
        
        auto signature = entityManager->getSignature(entity);
        signature.set(ComponentTypeCounter::getTypeID<T>(), false);
        entityManager->setSignature(entity, signature);
        
        systemManager->entitySignatureChanged(entity, signature);
    }
    
    template<typename T>
    T& getComponent(Entity entity)
	{
        return componentManager->getComponent<T>(entity);
    }
    
    template<typename T>
    bool hasComponent(Entity entity)
	{
        return componentManager->hasComponent<T>(entity);
    }
    
    // System methods
    template<typename T>
    std::shared_ptr<T> registerSystem()
	{
        return systemManager->registerSystem<T>();
    }
    
    template<typename T>
    void setSystemSignature(Signature signature)
	{
        systemManager->setSignature<T>(signature);
    }
    
    void update(float dt)
	{
        systemManager->update(dt);
    }
};

} // namespace ECS