#pragma once
#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Systems/PhysicsSystem.h"
#include <memory>
#include <functional>

class PhysicsManager
{
private:
    ECS::World* world;
    
    PhysicsSystem* physicsSystem;
    CollisionDetectionSystem* collisionDetectionSystem;
    CollisionResponseSystem* collisionResponseSystem;
    
public:
    PhysicsManager(ECS::World* ecsWorld) : world(ecsWorld) {
        // Register physics systems
        physicsSystem = world->RegisterSystem<PhysicsSystem>().get();
        physicsSystem->SetWorld(world);
        
        collisionDetectionSystem = world->RegisterSystem<CollisionDetectionSystem>().get();
        collisionDetectionSystem->SetWorld(world);
        
        collisionResponseSystem = world->RegisterSystem<CollisionResponseSystem>().get();
        collisionResponseSystem->SetWorld(world);
        
        // Set system signatures
        SetupSystemSignatures();
        
        LOG_INFO("PhysicsManager initialized");
    }
    
    ~PhysicsManager() = default;

	ECS::World* GetWorld() { return world; }
	
    // === ENTITY CREATION HELPERS ===
    
    // Create a physics entity with sphere collider
    ECS::Entity CreateSphereEntity(const glm::vec3& position, 
                                  float radius, 
                                  float mass = 1.0f,
                                  bool isStatic = false) {
        ECS::Entity entity = world->CreateEntity();
        
        // Add transform
        world->AddComponent(entity, TransformComponent(position));
        
        // Add rigidbody
        RigidBodyComponent rb(mass);
        rb.SetStatic(isStatic);
        world->AddComponent(entity, rb);
        
        // Add sphere collider
        world->AddComponent(entity, SphereColliderComponent(radius));
        
        // Add default physics material
        world->AddComponent(entity, PhysicsMaterialComponent());
        
        return entity;
    }
    
    // Create a physics entity with box collider
    ECS::Entity CreateBoxEntity(const glm::vec3& position,
                               const glm::vec3& halfExtents,
                               float mass = 1.0f,
                               bool isStatic = false) {
        ECS::Entity entity = world->CreateEntity();
        
        // Add transform
        world->AddComponent(entity, TransformComponent(position));
        
        // Add rigidbody
        RigidBodyComponent rb(mass);
        rb.SetStatic(isStatic);
        world->AddComponent(entity, rb);
        
        // Add box collider
        world->AddComponent(entity, BoxColliderComponent(halfExtents));
        
        // Add default physics material
        world->AddComponent(entity, PhysicsMaterialComponent());
        
        return entity;
    }
    
    // Create ground plane
    ECS::Entity CreateGroundPlane(const glm::vec3& position = glm::vec3(0, 0, 0),
                                 const glm::vec3& normal = glm::vec3(0, 1, 0)) {
        ECS::Entity entity = world->CreateEntity();
        
        // Add transform
        world->AddComponent(entity, TransformComponent(position));
        
        // Add static rigidbody
        RigidBodyComponent rb(1.0f);
        rb.SetStatic(true);
        world->AddComponent(entity, rb);
        
        // Add plane collider
        float distance = glm::dot(position, normal);
        world->AddComponent(entity, PlaneColliderComponent(normal, distance));
        
        // Add physics material
        PhysicsMaterialComponent material(0.8f, 0.2f); // High friction, low bounce
        world->AddComponent(entity, material);
        
        return entity;
    }
    
    // === PHYSICS CONTROL ===
    
    void SetGravity(const glm::vec3& gravity) {
        physicsSystem->SetGravity(gravity);
    }
    
    glm::vec3 GetGravity() const {
        return physicsSystem->GetSettings().gravity;
    }
    
    void AddForce(ECS::Entity entity, const glm::vec3& force) {
        physicsSystem->AddForce(entity, force);
    }
    
    void AddImpulse(ECS::Entity entity, const glm::vec3& impulse) {
        physicsSystem->AddImpulse(entity, impulse);
    }
    
    void SetVelocity(ECS::Entity entity, const glm::vec3& velocity) {
        physicsSystem->SetVelocity(entity, velocity);
    }
    
    glm::vec3 GetVelocity(ECS::Entity entity) {
        if (world->HasComponent<RigidBodyComponent>(entity)) {
            return world->GetComponent<RigidBodyComponent>(entity).velocity;
        }
        return glm::vec3(0.0f);
    }
    
    void SetMass(ECS::Entity entity, float mass) {
        if (world->HasComponent<RigidBodyComponent>(entity)) {
            world->GetComponent<RigidBodyComponent>(entity).SetMass(mass);
        }
    }
    
    void SetStatic(ECS::Entity entity, bool isStatic) {
        if (world->HasComponent<RigidBodyComponent>(entity)) {
            world->GetComponent<RigidBodyComponent>(entity).SetStatic(isStatic);
        }
    }
    
    void SetKinematic(ECS::Entity entity, bool isKinematic) {
        if (world->HasComponent<RigidBodyComponent>(entity)) {
            world->GetComponent<RigidBodyComponent>(entity).isKinematic = isKinematic;
        }
    }
    
    // === PHYSICS MATERIALS ===
    
    void SetMaterial(ECS::Entity entity, float friction, float restitution, float density = 1.0f) {
        if (world->HasComponent<PhysicsMaterialComponent>(entity)) {
            auto& material = world->GetComponent<PhysicsMaterialComponent>(entity);
            material.staticFriction = friction;
            material.dynamicFriction = friction * 0.8f;
            material.restitution = restitution;
            material.density = density;
        }
    }
    
    // === TRIGGER EVENTS ===
    
    void AddTriggerCallback(std::function<void(const CollisionInfo&)> callback) {
        collisionResponseSystem->AddTriggerCallback(callback);
    }
    
    void SetTrigger(ECS::Entity entity, bool isTrigger) {
        if (world->HasComponent<SphereColliderComponent>(entity)) {
            world->GetComponent<SphereColliderComponent>(entity).isTrigger = isTrigger;
        }
        if (world->HasComponent<BoxColliderComponent>(entity)) {
            world->GetComponent<BoxColliderComponent>(entity).isTrigger = isTrigger;
        }
        if (world->HasComponent<PlaneColliderComponent>(entity)) {
            world->GetComponent<PlaneColliderComponent>(entity).isTrigger = isTrigger;
        }
    }
    
    // === CONSTRAINTS ===
    
    void AddSpringConstraint(ECS::Entity entityA, ECS::Entity entityB, 
                           float restLength, float stiffness = 100.0f, float damping = 10.0f) {
        SpringConstraintComponent constraint;
        constraint.targetEntity = entityB;
        constraint.restLength = restLength;
        constraint.stiffness = stiffness;
        constraint.damping = damping;
        
        world->AddComponent(entityA, constraint);
    }
    
    // === SYSTEM UPDATE ===
    
    void Update(float deltaTime) {
        // Update physics systems in order
        world->Update(deltaTime);
        
        // Process collisions
        const auto& collisions = collisionDetectionSystem->GetCollisions();
        collisionResponseSystem->ProcessCollisions(collisions);
    }
    
    // === UTILITIES ===
    
    // Raycast (simple implementation)
    struct RaycastHit {
        bool hit = false;
        ECS::Entity entity;
        glm::vec3 point;
        glm::vec3 normal;
        float distance;
    };
    
    RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 1000.0f) {
        RaycastHit result;
        result.distance = maxDistance;
        
        for (auto entity : collisionDetectionSystem->Entities) {
            auto& transform = world->GetComponent<TransformComponent>(entity);
            
            // Check sphere colliders
            if (world->HasComponent<SphereColliderComponent>(entity)) {
                auto& sphere = world->GetComponent<SphereColliderComponent>(entity);
                glm::vec3 spherePos = transform.position + sphere.center;
                
                RaycastHit hit = RaycastSphere(origin, direction, spherePos, sphere.radius);
                if (hit.hit && hit.distance < result.distance) {
                    result = hit;
                    result.entity = entity;
                }
            }
            
            // Could add box raycast here
        }
        
        return result;
    }
    
    // === PHYSICS SETTINGS ===
    
    PhysicsWorldSettings& GetWorldSettings() {
        return physicsSystem->GetSettings();
    }
    
    // === DEBUG INFO ===
    
    struct PhysicsStats {
        int rigidBodyCount = 0;
        int colliderCount = 0;
        int activeCollisions = 0;
        glm::vec3 totalKineticEnergy = glm::vec3(0.0f);
    };
    
    PhysicsStats GetStats() {
        PhysicsStats stats;
        
        for (auto entity : physicsSystem->Entities) {
            stats.rigidBodyCount++;
            
            if (world->HasComponent<RigidBodyComponent>(entity)) {
                auto& rb = world->GetComponent<RigidBodyComponent>(entity);
                if (!rb.isStatic) {
                    float ke = 0.5f * rb.mass * glm::dot(rb.velocity, rb.velocity);
                    stats.totalKineticEnergy += glm::vec3(ke);
                }
            }
        }
        
        for (auto entity : collisionDetectionSystem->Entities) {
            stats.colliderCount++;
        }
        
        stats.activeCollisions = static_cast<int>(collisionDetectionSystem->GetCollisions().size());
        
        return stats;
    }
    
private:
    void SetupSystemSignatures() {
        // Physics system signature
        ECS::Signature physicsSignature;
        physicsSignature.set(ECS::ComponentTypeCounter::GetTypeId<TransformComponent>());
        physicsSignature.set(ECS::ComponentTypeCounter::GetTypeId<RigidBodyComponent>());
        world->SetSystemSignature<PhysicsSystem>(physicsSignature);
        
        // Collision detection system signature - entities with any collider
        ECS::Signature collisionSignature;
        collisionSignature.set(ECS::ComponentTypeCounter::GetTypeId<TransformComponent>());
        // Note: This signature will catch entities with any collider component
        // In a more complex system, you might want separate signatures for different collider types
        world->SetSystemSignature<CollisionDetectionSystem>(collisionSignature);
        
        // Collision response system doesn't need entities directly
        ECS::Signature responseSignature;
        world->SetSystemSignature<CollisionResponseSystem>(responseSignature);
    }
    
    RaycastHit RaycastSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
                           const glm::vec3& sphereCenter, float sphereRadius) {
        RaycastHit hit;
        
        glm::vec3 oc = rayOrigin - sphereCenter;
        float a = glm::dot(rayDirection, rayDirection);
        float b = 2.0f * glm::dot(oc, rayDirection);
        float c = glm::dot(oc, oc) - sphereRadius * sphereRadius;
        
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant >= 0) {
            float t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
            if (t > 0) {
                hit.hit = true;
                hit.distance = t;
                hit.point = rayOrigin + t * rayDirection;
                hit.normal = glm::normalize(hit.point - sphereCenter);
            }
        }
        
        return hit;
    }
};

// Utility functions for creating common physics scenarios
namespace PhysicsUtils {
    
    // Create a stack of boxes
    inline std::vector<ECS::Entity> CreateBoxStack(PhysicsManager& physics,
                                                   const glm::vec3& basePosition,
                                                   int count,
                                                   const glm::vec3& boxSize = glm::vec3(0.5f),
                                                   float spacing = 0.1f) {
        std::vector<ECS::Entity> entities;
        entities.reserve(count);
        
        for (int i = 0; i < count; ++i) {
            glm::vec3 pos = basePosition + glm::vec3(0, i * (boxSize.y * 2 + spacing), 0);
            auto entity = physics.CreateBoxEntity(pos, boxSize);
            entities.push_back(entity);
        }
        
        return entities;
    }
    
    // Create a ball pit
    inline std::vector<ECS::Entity> CreateBallPit(PhysicsManager& physics,
                                                 const glm::vec3& center,
                                                 const glm::vec3& bounds,
                                                 int ballCount,
                                                 float ballRadius = 0.2f) {
        std::vector<ECS::Entity> entities;
        entities.reserve(ballCount);
        
        for (int i = 0; i < ballCount; ++i) {
            glm::vec3 pos = center + glm::vec3(
                (rand() / float(RAND_MAX) - 0.5f) * bounds.x,
                (rand() / float(RAND_MAX) - 0.5f) * bounds.y,
                (rand() / float(RAND_MAX) - 0.5f) * bounds.z
            );
            
            auto entity = physics.CreateSphereEntity(pos, ballRadius);
            entities.push_back(entity);
        }
        
        return entities;
    }
    
    // Apply explosion force
    inline void ApplyExplosionForce(PhysicsManager& physics,
                                   const std::vector<ECS::Entity>& entities,
                                   const glm::vec3& explosionCenter,
                                   float explosionForce,
                                   float explosionRadius) {
        for (auto entity : entities) {
            auto velocity = physics.GetVelocity(entity);
            if (glm::length(velocity) < 0.1f) { // Only affect slow/stationary objects
                
                // Calculate direction and distance from explosion
                glm::vec3 entityPos = physics.GetWorld()->GetComponent<TransformComponent>(entity).position;
                glm::vec3 direction = entityPos - explosionCenter;
                float distance = glm::length(direction);
                
                if (distance < explosionRadius && distance > 0.0f) {
                    direction /= distance;
                    
                    // Falloff with distance
                    float forceMagnitude = explosionForce * (1.0f - distance / explosionRadius);
                    
                    physics.AddImpulse(entity, direction * forceMagnitude);
                }
            }
        }
    }
}