#pragma once
#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include <glm/glm.hpp>

// RigidBody Component - Fizik nesnesi özellikleri
struct RigidBodyComponent : public ECS::Component
{
    // Kinematic properties
    glm::vec3 velocity = glm::vec3(0.0f);
    glm::vec3 acceleration = glm::vec3(0.0f);
    glm::vec3 force = glm::vec3(0.0f);
    
    // Physical properties
    float mass = 1.0f;
    float inverseMass = 1.0f; // 1/mass, 0 for static objects
    float drag = 0.98f; // Air resistance (0.0 = no drag, 1.0 = full stop)
    float restitution = 0.5f; // Bounciness (0.0 = no bounce, 1.0 = perfect bounce)
    
    // State
    bool isStatic = false;
    bool useGravity = true;
    bool isKinematic = false; // Kinematic objects don't respond to forces
    
    RigidBodyComponent() = default;
    RigidBodyComponent(float m, bool gravity = true) 
        : mass(m), useGravity(gravity)
    {
        inverseMass = isStatic ? 0.0f : (mass > 0.0f ? 1.0f / mass : 0.0f);
    }
    
    void SetMass(float m) {
        mass = m;
        inverseMass = isStatic ? 0.0f : (mass > 0.0f ? 1.0f / mass : 0.0f);
    }
    
    void SetStatic(bool isStaticObj) {
        isStatic = isStaticObj;
        inverseMass = isStatic ? 0.0f : (mass > 0.0f ? 1.0f / mass : 0.0f);
    }
    
    void AddForce(const glm::vec3& f) {
        if (!isStatic && !isKinematic) {
            force += f;
        }
    }
    
    void AddImpulse(const glm::vec3& impulse) {
        if (!isStatic && !isKinematic) {
            velocity += impulse * inverseMass;
        }
    }
};

// Collider Types
enum class ColliderType {
    SPHERE,
    BOX,
    PLANE
};

// Base Collider Component
struct ColliderComponent : public ECS::Component
{
    ColliderType type = ColliderType::BOX;
    glm::vec3 center = glm::vec3(0.0f); // Local offset from transform position
    bool isTrigger = false; // Trigger colliders don't resolve physics, just detect
    
    // Material properties
    float friction = 0.5f;
    float density = 1.0f;
    
    ColliderComponent() = default;
    ColliderComponent(ColliderType t) : type(t) {}
};

// Sphere Collider
struct SphereColliderComponent : public ColliderComponent
{
    float radius = 0.5f;
    
    SphereColliderComponent() {
        type = ColliderType::SPHERE;
    }
    
    SphereColliderComponent(float r, const glm::vec3& offset = glm::vec3(0.0f)) 
        : radius(r) {
        type = ColliderType::SPHERE;
        center = offset;
    }
};

// Box Collider
struct BoxColliderComponent : public ColliderComponent
{
    glm::vec3 size = glm::vec3(1.0f); // Half extents
    
    BoxColliderComponent() {
        type = ColliderType::BOX;
    }
    
    BoxColliderComponent(const glm::vec3& halfExtents, const glm::vec3& offset = glm::vec3(0.0f)) 
        : size(halfExtents) {
        type = ColliderType::BOX;
        center = offset;
    }
};

// Plane Collider (infinite plane)
struct PlaneColliderComponent : public ColliderComponent
{
    glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f); // Plane normal
    float distance = 0.0f; // Distance from origin along normal
    
    PlaneColliderComponent() {
        type = ColliderType::PLANE;
    }
    
    PlaneColliderComponent(const glm::vec3& n, float d = 0.0f) 
        : normal(glm::normalize(n)), distance(d) {
        type = ColliderType::PLANE;
    }
};

// Collision Information
struct CollisionInfo
{
    ECS::Entity entityA;
    ECS::Entity entityB;
    glm::vec3 contactPoint;
    glm::vec3 contactNormal;
    float penetrationDepth;
    bool isTrigger = false;
};

// Physics Material
struct PhysicsMaterialComponent : public ECS::Component
{
    float staticFriction = 0.6f;
    float dynamicFriction = 0.4f;
    float restitution = 0.4f;
    float density = 1.0f;
    
    PhysicsMaterialComponent() = default;
    PhysicsMaterialComponent(float friction, float bounce, float dens = 1.0f)
        : staticFriction(friction), dynamicFriction(friction * 0.8f), 
          restitution(bounce), density(dens) {}
};

// Physics Constraints (simple)
struct SpringConstraintComponent : public ECS::Component
{
    ECS::Entity targetEntity = 0;
    glm::vec3 localAnchorA = glm::vec3(0.0f);
    glm::vec3 localAnchorB = glm::vec3(0.0f);
    float restLength = 1.0f;
    float stiffness = 100.0f;
    float damping = 10.0f;
    
    SpringConstraintComponent() = default;
    SpringConstraintComponent(ECS::Entity target, float length, float k = 100.0f)
        : targetEntity(target), restLength(length), stiffness(k) {}
};