#pragma once
#include "ECS/ECS.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/PhysicsComponent.h"
#include "ECS/Systems/PhysicsSystem.h"
#include <memory>
#include <functional>

#include "Logger.h"

class PhysicsManager
{
private:
    ECS::World* world;
    
    PhysicsSystem* physicsSystem;
    CollisionDetectionSystem* collisionDetectionSystem;
    CollisionResponseSystem* collisionResponseSystem;
    
public:
    PhysicsManager(ECS::World* ecsWorld);

    ~PhysicsManager() = default;

	ECS::World* GetWorld() { return world; }
	
    // === ENTITY CREATION HELPERS ===
    
    // Create a physics entity with sphere collider
    ECS::Entity CreateSphereEntity(const glm::vec3& position, 
                                  float radius, 
                                  float mass = 1.0f,
                                  bool isStatic = false) const;

	// Create a physics entity with box collider
    ECS::Entity CreateBoxEntity(const glm::vec3& position,
                               const glm::vec3& halfExtents,
                               float mass = 1.0f,
                               bool isStatic = false) const;

	// Create ground plane
    ECS::Entity CreateGroundPlane(const glm::vec3& position = glm::vec3(0, 0, 0),
                                 const glm::vec3& normal = glm::vec3(0, 1, 0)) const;

	// === PHYSICS CONTROL ===
    
    void SetGravity(const glm::vec3& gravity) const;

	glm::vec3 GetGravity() const;

	void AddForce(ECS::Entity entity, const glm::vec3& force) const;

	void AddImpulse(ECS::Entity entity, const glm::vec3& impulse) const;

	void SetVelocity(ECS::Entity entity, const glm::vec3& velocity) const;

	glm::vec3 GetVelocity(ECS::Entity entity) const;

	void SetMass(ECS::Entity entity, float mass) const;

	void SetStatic(ECS::Entity entity, bool isStatic) const;

	void SetKinematic(ECS::Entity entity, bool isKinematic) const;

	// === PHYSICS MATERIALS ===
    
    void SetMaterial(ECS::Entity entity, float friction, float restitution, float density = 1.0f) const;

	// === TRIGGER EVENTS ===
    
    void AddTriggerCallback(std::function<void(const CollisionInfo&)> callback) const;

	void SetTrigger(ECS::Entity entity, bool isTrigger) const;

	// === CONSTRAINTS ===
    
    void AddSpringConstraint(ECS::Entity entityA, ECS::Entity entityB, 
                           float restLength, float stiffness = 100.0f, float damping = 10.0f) const;

	// === SYSTEM UPDATE ===
    
    void Update(float deltaTime) const;

	// === UTILITIES ===
    
    // Raycast (simple implementation)
    struct RaycastHit {
        bool hit = false;
        ECS::Entity entity;
        glm::vec3 point;
        glm::vec3 normal;
        float distance;
    };
    
    RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float maxDistance = 1000.0f);

	// === PHYSICS SETTINGS ===
    
    PhysicsWorldSettings& GetWorldSettings() const;

	// === DEBUG INFO ===
    
    struct PhysicsStats {
        int rigidBodyCount = 0;
        int colliderCount = 0;
        int activeCollisions = 0;
        glm::vec3 totalKineticEnergy = glm::vec3(0.0f);
    };
    
    PhysicsStats GetStats() const;

private:
    void SetupSystemSignatures() const;

	RaycastHit RaycastSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
							const glm::vec3& sphereCenter, float sphereRadius);
};

// Utility functions for creating common physics scenarios
namespace PhysicsUtils {
    
    // Create a stack of boxes
	std::vector<ECS::Entity> CreateBoxStack(PhysicsManager& physics,
											const glm::vec3& basePosition,
											int count,
											const glm::vec3& boxSize = glm::vec3(0.5f),
											float spacing = 0.1f);

	// Create a ball pit
	std::vector<ECS::Entity> CreateBallPit(PhysicsManager& physics,
											const glm::vec3& center,
											const glm::vec3& bounds,
											int ballCount,
											float ballRadius = 0.2f);

	// Apply explosion force
	void ApplyExplosionForce(PhysicsManager& physics,
							const std::vector<ECS::Entity>& entities,
							const glm::vec3& explosionCenter,
							float explosionForce,
							float explosionRadius);
}
