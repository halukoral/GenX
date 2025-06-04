#pragma once
#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include <vector>
#include <functional>

// Physics World Settings
struct PhysicsWorldSettings
{
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
    float timeStep = 1.0f / 60.0f; // Fixed timestep
    int velocityIterations = 8;
    int positionIterations = 3;
    bool enableSleeping = true;
    float sleepThreshold = 0.1f;
};

// Physics Integration System
class PhysicsSystem : public ECS::System
{
private:
    ECS::World* world;
    PhysicsWorldSettings settings;
    float accumulator = 0.0f;
    
public:
    PhysicsSystem() = default;
    
    void SetWorld(ECS::World* w) { world = w; }
    
    void SetGravity(const glm::vec3& gravity) {
        settings.gravity = gravity;
    }
    
    const PhysicsWorldSettings& GetSettings() const { return settings; }
    PhysicsWorldSettings& GetSettings() { return settings; }
    
    void Update(float dt) override {
        accumulator += dt;
        
        // Fixed timestep physics
        while (accumulator >= settings.timeStep) {
            Step(settings.timeStep);
            accumulator -= settings.timeStep;
        }
    }
    
    void Step(float deltaTime) {
        // Apply forces and integrate
        for (auto entity : Entities) {
            auto& rigidBody = world->GetComponent<RigidBodyComponent>(entity);
            auto& transform = world->GetComponent<TransformComponent>(entity);
            
            if (rigidBody.isStatic) continue;
            
            // Apply gravity
            if (rigidBody.useGravity && !rigidBody.isKinematic) {
                rigidBody.AddForce(settings.gravity * rigidBody.mass);
            }
            
            // Integrate forces -> acceleration
            if (!rigidBody.isKinematic) {
                rigidBody.acceleration = rigidBody.force * rigidBody.inverseMass;
            }
            
            // Integrate acceleration -> velocity
            rigidBody.velocity += rigidBody.acceleration * deltaTime;
            
            // Apply drag
            rigidBody.velocity *= glm::pow(rigidBody.drag, deltaTime);
            
            // Integrate velocity -> position
            if (!rigidBody.isStatic) {
                transform.position += rigidBody.velocity * deltaTime;
            }
            
            // Clear forces for next frame
            rigidBody.force = glm::vec3(0.0f);
        }
    }
    
    // Add force to entity
    void AddForce(ECS::Entity entity, const glm::vec3& force) {
        if (world->HasComponent<RigidBodyComponent>(entity)) {
            world->GetComponent<RigidBodyComponent>(entity).AddForce(force);
        }
    }
    
    // Add impulse to entity
    void AddImpulse(ECS::Entity entity, const glm::vec3& impulse) {
        if (world->HasComponent<RigidBodyComponent>(entity)) {
            world->GetComponent<RigidBodyComponent>(entity).AddImpulse(impulse);
        }
    }
    
    // Set velocity directly
    void SetVelocity(ECS::Entity entity, const glm::vec3& velocity) {
        if (world->HasComponent<RigidBodyComponent>(entity)) {
            auto& rb = world->GetComponent<RigidBodyComponent>(entity);
            if (!rb.isStatic) {
                rb.velocity = velocity;
            }
        }
    }
};

// Collision Detection System
class CollisionDetectionSystem : public ECS::System
{
private:
    ECS::World* world;
    std::vector<CollisionInfo> collisions;
    
public:
    CollisionDetectionSystem() = default;
    
    void SetWorld(ECS::World* w) { world = w; }
    
    void Update(float dt) override {
        collisions.clear();
        
        // Broad phase - check all pairs (O(n²) - could be optimized with spatial partitioning)
        for (size_t i = 0; i < Entities.size(); ++i) {
            for (size_t j = i + 1; j < Entities.size(); ++j) {
                CheckCollision(Entities[i], Entities[j]);
            }
        }
    }
    
    const std::vector<CollisionInfo>& GetCollisions() const { return collisions; }
    
private:
    void CheckCollision(ECS::Entity entityA, ECS::Entity entityB) {
        auto& transformA = world->GetComponent<TransformComponent>(entityA);
        auto& transformB = world->GetComponent<TransformComponent>(entityB);
        
        // Get world positions
        glm::vec3 posA = transformA.position;
        glm::vec3 posB = transformB.position;
        
        CollisionInfo collision;
        collision.entityA = entityA;
        collision.entityB = entityB;
        
        // Sphere-Sphere collision
        if (world->HasComponent<SphereColliderComponent>(entityA) && 
            world->HasComponent<SphereColliderComponent>(entityB)) {
            
            auto& sphereA = world->GetComponent<SphereColliderComponent>(entityA);
            auto& sphereB = world->GetComponent<SphereColliderComponent>(entityB);
            
            glm::vec3 worldPosA = posA + sphereA.center;
            glm::vec3 worldPosB = posB + sphereB.center;
            
            if (CheckSphereSphere(worldPosA, sphereA.radius, worldPosB, sphereB.radius, collision)) {
                collision.isTrigger = sphereA.isTrigger || sphereB.isTrigger;
                collisions.push_back(collision);
            }
        }
        // Box-Box collision (simplified AABB)
        else if (world->HasComponent<BoxColliderComponent>(entityA) && 
                 world->HasComponent<BoxColliderComponent>(entityB)) {
            
            auto& boxA = world->GetComponent<BoxColliderComponent>(entityA);
            auto& boxB = world->GetComponent<BoxColliderComponent>(entityB);
            
            glm::vec3 worldPosA = posA + boxA.center;
            glm::vec3 worldPosB = posB + boxB.center;
            
            if (CheckAABBAABB(worldPosA, boxA.size, worldPosB, boxB.size, collision)) {
                collision.isTrigger = boxA.isTrigger || boxB.isTrigger;
                collisions.push_back(collision);
            }
        }
        // Sphere-Box collision
        else if ((world->HasComponent<SphereColliderComponent>(entityA) && 
                  world->HasComponent<BoxColliderComponent>(entityB)) ||
                 (world->HasComponent<BoxColliderComponent>(entityA) && 
                  world->HasComponent<SphereColliderComponent>(entityB))) {
            
            // Ensure A is sphere, B is box
            if (world->HasComponent<BoxColliderComponent>(entityA)) {
                std::swap(entityA, entityB);
                std::swap(posA, posB);
            }
            
            auto& sphere = world->GetComponent<SphereColliderComponent>(entityA);
            auto& box = world->GetComponent<BoxColliderComponent>(entityB);
            
            glm::vec3 spherePos = posA + sphere.center;
            glm::vec3 boxPos = posB + box.center;
            
            if (CheckSphereAABB(spherePos, sphere.radius, boxPos, box.size, collision)) {
                collision.isTrigger = sphere.isTrigger || box.isTrigger;
                collisions.push_back(collision);
            }
        }
        // Plane collisions
        else if (world->HasComponent<PlaneColliderComponent>(entityA) || 
                 world->HasComponent<PlaneColliderComponent>(entityB)) {
            
            // Ensure A is not plane, B is plane
            if (world->HasComponent<PlaneColliderComponent>(entityA)) {
                std::swap(entityA, entityB);
                std::swap(posA, posB);
            }
            
            auto& plane = world->GetComponent<PlaneColliderComponent>(entityB);
            
            // Check sphere-plane
            if (world->HasComponent<SphereColliderComponent>(entityA)) {
                auto& sphere = world->GetComponent<SphereColliderComponent>(entityA);
                glm::vec3 spherePos = posA + sphere.center;
                
                if (CheckSpherePlane(spherePos, sphere.radius, plane.normal, plane.distance, collision)) {
                    collision.isTrigger = sphere.isTrigger || plane.isTrigger;
                    collisions.push_back(collision);
                }
            }
        }
    }
    
    bool CheckSphereSphere(const glm::vec3& posA, float radiusA, 
                          const glm::vec3& posB, float radiusB, 
                          CollisionInfo& collision) {
        glm::vec3 direction = posB - posA;
        float distance = glm::length(direction);
        float radiusSum = radiusA + radiusB;
        
        if (distance < radiusSum && distance > 0.0f) {
            collision.contactNormal = direction / distance;
            collision.contactPoint = posA + collision.contactNormal * radiusA;
            collision.penetrationDepth = radiusSum - distance;
            return true;
        }
        return false;
    }
    
    bool CheckAABBAABB(const glm::vec3& posA, const glm::vec3& sizeA,
                       const glm::vec3& posB, const glm::vec3& sizeB,
                       CollisionInfo& collision) {
        glm::vec3 minA = posA - sizeA;
        glm::vec3 maxA = posA + sizeA;
        glm::vec3 minB = posB - sizeB;
        glm::vec3 maxB = posB + sizeB;
        
        if (maxA.x < minB.x || minA.x > maxB.x ||
            maxA.y < minB.y || minA.y > maxB.y ||
            maxA.z < minB.z || minA.z > maxB.z) {
            return false;
        }
        
        // Calculate overlap and contact normal
        glm::vec3 overlap = glm::min(maxA, maxB) - glm::max(minA, minB);
        
        // Find minimum overlap axis
        if (overlap.x <= overlap.y && overlap.x <= overlap.z) {
            collision.contactNormal = (posA.x < posB.x) ? glm::vec3(-1, 0, 0) : glm::vec3(1, 0, 0);
            collision.penetrationDepth = overlap.x;
        } else if (overlap.y <= overlap.z) {
            collision.contactNormal = (posA.y < posB.y) ? glm::vec3(0, -1, 0) : glm::vec3(0, 1, 0);
            collision.penetrationDepth = overlap.y;
        } else {
            collision.contactNormal = (posA.z < posB.z) ? glm::vec3(0, 0, -1) : glm::vec3(0, 0, 1);
            collision.penetrationDepth = overlap.z;
        }
        
        collision.contactPoint = (posA + posB) * 0.5f;
        return true;
    }
    
    bool CheckSphereAABB(const glm::vec3& spherePos, float radius,
                        const glm::vec3& boxPos, const glm::vec3& boxSize,
                        CollisionInfo& collision) {
        glm::vec3 closest = glm::clamp(spherePos, boxPos - boxSize, boxPos + boxSize);
        glm::vec3 direction = spherePos - closest;
        float distance = glm::length(direction);
        
        if (distance < radius) {
            if (distance > 0.0f) {
                collision.contactNormal = direction / distance;
            } else {
                // Sphere center inside box
                collision.contactNormal = glm::vec3(0, 1, 0);
            }
            collision.contactPoint = closest;
            collision.penetrationDepth = radius - distance;
            return true;
        }
        return false;
    }
    
    bool CheckSpherePlane(const glm::vec3& spherePos, float radius,
                         const glm::vec3& planeNormal, float planeDistance,
                         CollisionInfo& collision) {
        float distanceToPlane = glm::dot(spherePos, planeNormal) - planeDistance;
        
        if (distanceToPlane < radius) {
            collision.contactNormal = -planeNormal;
            collision.contactPoint = spherePos - planeNormal * radius;
            collision.penetrationDepth = radius - distanceToPlane;
            return true;
        }
        return false;
    }
};

// Collision Response System
class CollisionResponseSystem : public ECS::System
{
private:
    ECS::World* world;
    std::vector<std::function<void(const CollisionInfo&)>> triggerCallbacks;
    
public:
    CollisionResponseSystem() = default;
    
    void SetWorld(ECS::World* w) { world = w; }
    
    void Update(float dt) override {
        // This system doesn't need regular updates
        // It processes collisions when they're reported
    }
    
    void ProcessCollisions(const std::vector<CollisionInfo>& collisions) {
        for (const auto& collision : collisions) {
            if (collision.isTrigger) {
                // Handle trigger collision
                HandleTriggerCollision(collision);
            } else {
                // Handle physical collision
                ResolveCollision(collision);
            }
        }
    }
    
    void AddTriggerCallback(std::function<void(const CollisionInfo&)> callback) {
        triggerCallbacks.push_back(callback);
    }
    
private:
    void ResolveCollision(const CollisionInfo& collision) {
        if (!world->HasComponent<RigidBodyComponent>(collision.entityA) ||
            !world->HasComponent<RigidBodyComponent>(collision.entityB)) {
            return;
        }
        
        auto& rbA = world->GetComponent<RigidBodyComponent>(collision.entityA);
        auto& rbB = world->GetComponent<RigidBodyComponent>(collision.entityB);
        auto& transformA = world->GetComponent<TransformComponent>(collision.entityA);
        auto& transformB = world->GetComponent<TransformComponent>(collision.entityB);
        
        // Position correction (separate overlapping objects)
        float totalInverseMass = rbA.inverseMass + rbB.inverseMass;
        if (totalInverseMass > 0.0f) {
            float correctionPercent = 0.8f; // How much to correct (0-1)
            float slop = 0.01f; // Penetration allowance
            glm::vec3 correction = (glm::max(collision.penetrationDepth - slop, 0.0f) / totalInverseMass) * 
                                 correctionPercent * collision.contactNormal;
            
            transformA.position -= correction * rbA.inverseMass;
            transformB.position += correction * rbB.inverseMass;
        }
        
        // Impulse resolution
        glm::vec3 relativeVelocity = rbB.velocity - rbA.velocity;
        float velocityAlongNormal = glm::dot(relativeVelocity, collision.contactNormal);
        
        // Objects separating
        if (velocityAlongNormal > 0) return;
        
        // Calculate restitution
        float restitution = glm::min(rbA.restitution, rbB.restitution);
        
        // Calculate impulse scalar
        float impulseScalar = -(1 + restitution) * velocityAlongNormal;
        impulseScalar /= totalInverseMass;
        
        // Apply impulse
        glm::vec3 impulse = impulseScalar * collision.contactNormal;
        rbA.velocity -= impulse * rbA.inverseMass;
        rbB.velocity += impulse * rbB.inverseMass;
        
        // Friction (simplified)
        ApplyFriction(collision, rbA, rbB, impulseScalar);
    }
    
    void ApplyFriction(const CollisionInfo& collision, 
                      RigidBodyComponent& rbA, RigidBodyComponent& rbB, 
                      float normalImpulse) {
        glm::vec3 relativeVelocity = rbB.velocity - rbA.velocity;
        glm::vec3 tangent = relativeVelocity - glm::dot(relativeVelocity, collision.contactNormal) * collision.contactNormal;
        
        float tangentLength = glm::length(tangent);
        if (tangentLength < 1e-6f) return;
        
        tangent /= tangentLength;
        
        // Get friction values
        float staticFriction = 0.6f;
        float dynamicFriction = 0.4f;
        
        if (world->HasComponent<PhysicsMaterialComponent>(collision.entityA)) {
            auto& matA = world->GetComponent<PhysicsMaterialComponent>(collision.entityA);
            staticFriction = matA.staticFriction;
            dynamicFriction = matA.dynamicFriction;
        }
        
        float totalInverseMass = rbA.inverseMass + rbB.inverseMass;
        float frictionImpulse = -glm::dot(relativeVelocity, tangent) / totalInverseMass;
        
        glm::vec3 friction;
        if (glm::abs(frictionImpulse) < glm::abs(normalImpulse) * staticFriction) {
            friction = frictionImpulse * tangent;
        } else {
            friction = -glm::abs(normalImpulse) * dynamicFriction * tangent;
        }
        
        rbA.velocity -= friction * rbA.inverseMass;
        rbB.velocity += friction * rbB.inverseMass;
    }
    
    void HandleTriggerCollision(const CollisionInfo& collision) {
        // Call all registered trigger callbacks
        for (auto& callback : triggerCallbacks) {
            callback(collision);
        }
    }
};