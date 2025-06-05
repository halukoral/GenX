#include "PhysicsManager.h"

PhysicsManager::PhysicsManager(ECS::World* ecsWorld): world(ecsWorld)
{
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

ECS::Entity PhysicsManager::CreateSphereEntity(const glm::vec3& position, float radius, float mass, bool isStatic) const
{
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

ECS::Entity PhysicsManager::CreateBoxEntity(const glm::vec3& position, const glm::vec3& halfExtents, float mass,
	bool isStatic) const
{
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

ECS::Entity PhysicsManager::CreateGroundPlane(const glm::vec3& position, const glm::vec3& normal) const
{
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

void PhysicsManager::SetGravity(const glm::vec3& gravity) const
{
	physicsSystem->SetGravity(gravity);
}

glm::vec3 PhysicsManager::GetGravity() const
{
	return physicsSystem->GetSettings().gravity;
}

void PhysicsManager::AddForce(ECS::Entity entity, const glm::vec3& force) const
{
	physicsSystem->AddForce(entity, force);
}

void PhysicsManager::AddImpulse(ECS::Entity entity, const glm::vec3& impulse) const
{
	physicsSystem->AddImpulse(entity, impulse);
}

void PhysicsManager::SetVelocity(ECS::Entity entity, const glm::vec3& velocity) const
{
	physicsSystem->SetVelocity(entity, velocity);
}

glm::vec3 PhysicsManager::GetVelocity(ECS::Entity entity) const
{
	if (world->HasComponent<RigidBodyComponent>(entity)) {
		return world->GetComponent<RigidBodyComponent>(entity).velocity;
	}
	return glm::vec3(0.0f);
}

void PhysicsManager::SetMass(ECS::Entity entity, float mass) const
{
	if (world->HasComponent<RigidBodyComponent>(entity)) {
		world->GetComponent<RigidBodyComponent>(entity).SetMass(mass);
	}
}

void PhysicsManager::SetStatic(ECS::Entity entity, bool isStatic) const
{
	if (world->HasComponent<RigidBodyComponent>(entity)) {
		world->GetComponent<RigidBodyComponent>(entity).SetStatic(isStatic);
	}
}

void PhysicsManager::SetKinematic(ECS::Entity entity, bool isKinematic) const
{
	if (world->HasComponent<RigidBodyComponent>(entity)) {
		world->GetComponent<RigidBodyComponent>(entity).isKinematic = isKinematic;
	}
}

void PhysicsManager::SetMaterial(ECS::Entity entity, float friction, float restitution, float density) const
{
	if (world->HasComponent<PhysicsMaterialComponent>(entity)) {
		auto& material = world->GetComponent<PhysicsMaterialComponent>(entity);
		material.staticFriction = friction;
		material.dynamicFriction = friction * 0.8f;
		material.restitution = restitution;
		material.density = density;
	}
}

void PhysicsManager::AddTriggerCallback(std::function<void(const CollisionInfo&)> callback) const
{
	collisionResponseSystem->AddTriggerCallback(callback);
}

void PhysicsManager::SetTrigger(ECS::Entity entity, bool isTrigger) const
{
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

void PhysicsManager::AddSpringConstraint(ECS::Entity entityA, ECS::Entity entityB, float restLength, float stiffness,
	float damping) const
{
	SpringConstraintComponent constraint;
	constraint.targetEntity = entityB;
	constraint.restLength = restLength;
	constraint.stiffness = stiffness;
	constraint.damping = damping;
        
	world->AddComponent(entityA, constraint);
}

void PhysicsManager::Update(float deltaTime) const
{
	// Update physics systems in order
	world->Update(deltaTime);
        
	// Process collisions
	const auto& collisions = collisionDetectionSystem->GetCollisions();
	collisionResponseSystem->ProcessCollisions(collisions);
}

PhysicsManager::RaycastHit PhysicsManager::Raycast(const glm::vec3& origin, const glm::vec3& direction,
	float maxDistance)
{
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

PhysicsWorldSettings& PhysicsManager::GetWorldSettings() const
{
	return physicsSystem->GetSettings();
}

PhysicsManager::PhysicsStats PhysicsManager::GetStats() const
{
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

void PhysicsManager::SetupSystemSignatures() const
{
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

PhysicsManager::RaycastHit PhysicsManager::RaycastSphere(const glm::vec3& rayOrigin, const glm::vec3& rayDirection,
	const glm::vec3& sphereCenter, float sphereRadius)
{
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

std::vector<ECS::Entity> PhysicsUtils::CreateBoxStack(PhysicsManager& physics, const glm::vec3& basePosition, int count,
	const glm::vec3& boxSize, float spacing)
{
	std::vector<ECS::Entity> entities;
	entities.reserve(count);
        
	for (int i = 0; i < count; ++i) {
		glm::vec3 pos = basePosition + glm::vec3(0, i * (boxSize.y * 2 + spacing), 0);
		auto entity = physics.CreateBoxEntity(pos, boxSize);
		entities.push_back(entity);
	}
        
	return entities;
}

std::vector<ECS::Entity> PhysicsUtils::CreateBallPit(PhysicsManager& physics, const glm::vec3& center,
	const glm::vec3& bounds, int ballCount, float ballRadius)
{
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

void PhysicsUtils::ApplyExplosionForce(PhysicsManager& physics, const std::vector<ECS::Entity>& entities,
	const glm::vec3& explosionCenter, float explosionForce, float explosionRadius)
{
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
