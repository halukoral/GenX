// PhysicsLayer.cpp
#include "PhysicsLayer.h"
#include "Application.h"
#include "Event/KeyEvent.h"
#include "Input/Input.h"
#include <random>

#include "Renderer/PrimitiveModels.h"

void PhysicsLayer::OnAttach()
{
    LOG_INFO("PhysicsLayer::OnAttach called");
    
    // Get reference to ModelLayer from Application
    const auto& app = Application::Get();
    modelLayer = app.GetModelLayer();
    
    if (!modelLayer) {
        LOG_ERROR("ModelLayer not found! Physics system cannot work.");
        return;
    }
    
    LOG_INFO("ModelLayer found: {}", (void*)modelLayer);
    
    sharedWorld = modelLayer->GetModelManager()->GetWorld();
    
    if (!sharedWorld) {
        LOG_ERROR("Shared ECS World not found!");
        return;
    }
    
    LOG_INFO("Shared world found: {}", (void*)sharedWorld);

    const auto renderer = app.GetRenderer();
    if (!renderer) {
        LOG_ERROR("Renderer not found!");
        return;
    }
    
    Device* device = renderer->GetDevice();
    if (!device) {
        LOG_ERROR("Device not found!");
        return;
    }
    
    LOG_INFO("Device found: {}", (void*)device);
    
    // Initialize primitive models FIRST
    LOG_INFO("Initializing primitive models...");
    PrimitiveModels::Initialize(device);
    
    // Create primitive model entities
    LOG_INFO("Creating primitive model entities...");
    
    cubeEntity = PrimitiveModels::CreateCube(sharedWorld, 0.5f);
    if (cubeEntity == 0) {
        LOG_ERROR("Failed to create cube entity!");
    } else {
        LOG_INFO("Cube entity created: {}", cubeEntity);
        
        // Verify cube entity has ModelComponent
        if (!sharedWorld->HasComponent<ModelComponent>(cubeEntity)) {
            LOG_ERROR("Cube entity {} is missing ModelComponent!", cubeEntity);
        } else {
            auto& modelComp = sharedWorld->GetComponent<ModelComponent>(cubeEntity);
            LOG_INFO("Cube entity {} has ModelComponent with {} meshes", 
                    cubeEntity, modelComp.ModelData ? modelComp.ModelData->Meshes.size() : 0);
        }
    }
    
    sphereEntity = PrimitiveModels::CreateSphere(sharedWorld, 0.5f, 32, 12);
    if (sphereEntity == 0) {
        LOG_ERROR("Failed to create sphere entity!");
    } else {
        LOG_INFO("Sphere entity created: {}", sphereEntity);
    }
    
    // Create physics manager
    physicsManager = std::make_unique<PhysicsManager>(sharedWorld);
    
    // Set up physics world
    physicsManager->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    
    // Create ground plane
    auto ground = physicsManager->CreateGroundPlane(glm::vec3(0, -2, 0));
    physicsManager->SetMaterial(ground, 0.8f, 0.3f);
    
    // Create demo scene if enabled
    if (demoMode) {
        CreatePhysicsDemo();
    }
    
    LOG_INFO("PhysicsLayer attached successfully");
}

void PhysicsLayer::OnDetach()
{
	LOG_INFO("PhysicsLayer detached");
    
	ClearDemo();
    
	// Clean up primitive entities
	if (sharedWorld)
	{
		if (cubeEntity != 0)
		{
			sharedWorld->DestroyEntity(cubeEntity);
		}
		if (sphereEntity != 0)
		{
			sharedWorld->DestroyEntity(sphereEntity);
		}
	}
    
	if (physicsManager)
	{
		physicsManager.reset();
	}
    
	// World'ü silmiyoruz çünkü ModelLayer'a ait
	sharedWorld = nullptr;
}

void PhysicsLayer::OnUpdate(float ts)
{
    if (physicsManager)
    {
        physicsManager->Update(ts);
        
        // Demo controls
        if (demoMode)
        {
            HandleDemoInput();
        }
    }
}

void PhysicsLayer::OnEvent(Event& event)
{
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e)
    {
        if (e.GetKeyCode() == KeyCode::P)
        {
            // Toggle physics demo
            demoMode = !demoMode;
            if (demoMode)
            {
                CreatePhysicsDemo();
            }
        	else
        	{
                ClearDemo();
            }
            return true;
        }
        
        if (demoMode)
        {
            if (e.GetKeyCode() == KeyCode::Space)
            {
                // Add random objects
                AddRandomPhysicsObjects();
                return true;
            }
            
            if (e.GetKeyCode() == KeyCode::B)
            {
                // Add explosion at center
                AddExplosion(glm::vec3(0, 5, 0), 15.0f, 8.0f);
                return true;
            }
            
            if (e.GetKeyCode() == KeyCode::C)
            {
                // Clear demo objects
                ClearDemo();
                return true;
            }
        }
        
        return false;
    });
}

ECS::Entity PhysicsLayer::CreatePhysicsBox(const glm::vec3& position, const glm::vec3& size, float mass) const
{
	LOG_DEBUG("=== CreatePhysicsBox START ===");
    
	if (!physicsManager || !sharedWorld)
	{
		LOG_ERROR("PhysicsManager or SharedWorld is null!");
		return 0;
	}
    
	auto entity = physicsManager->CreateBoxEntity(position, size, mass);
	physicsManager->SetMaterial(entity, 0.6f, 0.4f);
    
	// Get cube model data from the primitive cube entity
	if (cubeEntity != 0 && sharedWorld->HasComponent<ModelComponent>(cubeEntity))
	{
		auto& cubeModelComp = sharedWorld->GetComponent<ModelComponent>(cubeEntity);
        
		// Copy model data to physics entity
		ModelComponent modelComp;
		modelComp.ModelData = cubeModelComp.ModelData;  // Share the same mesh data
		modelComp.IsLoaded = true;
		modelComp.IsDirty = true;  // Needs its own buffers
		modelComp.ModelPath = "primitive://cube_instance";
        
		sharedWorld->AddComponent(entity, modelComp);
		sharedWorld->AddComponent(entity, RenderableComponent(true));
		sharedWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.7f, 0.3f, 0.2f)));
        
		// Scale to match physics collider
		if (sharedWorld->HasComponent<TransformComponent>(entity))
		{
			auto& transform = sharedWorld->GetComponent<TransformComponent>(entity);
			transform.scale = size * 2.0f;
		}
        
		LOG_INFO("Created physics box with visual at ({}, {}, {})", position.x, position.y, position.z);
	}
    
	return entity;
}

ECS::Entity PhysicsLayer::CreatePhysicsSphere(const glm::vec3& position, float radius, float mass) const
{
	LOG_DEBUG("=== CreatePhysicsSphere START ===");
    
	if (!physicsManager || !sharedWorld)
	{
		LOG_ERROR("PhysicsManager or SharedWorld is null!");
		return 0;
	}
    
	auto entity = physicsManager->CreateSphereEntity(position, radius, mass);
	physicsManager->SetMaterial(entity, 0.4f, 0.7f);
    
	// Get sphere model data from the primitive sphere entity
	if (sphereEntity != 0 && sharedWorld->HasComponent<ModelComponent>(sphereEntity))
	{
		auto& sphereModelComp = sharedWorld->GetComponent<ModelComponent>(sphereEntity);
        
		// Copy model data to physics entity
		ModelComponent modelComp;
		modelComp.ModelData = sphereModelComp.ModelData;  // Share the same mesh data
		modelComp.IsLoaded = true;
		modelComp.IsDirty = true;  // Needs its own buffers
		modelComp.ModelPath = "primitive://sphere_instance";
        
		sharedWorld->AddComponent(entity, modelComp);
		sharedWorld->AddComponent(entity, RenderableComponent(true));
		sharedWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.2f, 0.7f, 0.3f)));
        
		// Scale to match physics collider
		if (sharedWorld->HasComponent<TransformComponent>(entity))
		{
			auto& transform = sharedWorld->GetComponent<TransformComponent>(entity);
			transform.scale = glm::vec3(radius * 2.0f);
		}
        
		LOG_INFO("Created physics sphere with visual at ({}, {}, {})", position.x, position.y, position.z);
	}
    
	return entity;
}

void PhysicsLayer::AddExplosion(const glm::vec3& position, float force, float radius) const
{
    if (!physicsManager) return;
    
    PhysicsUtils::ApplyExplosionForce(*physicsManager, demoEntities, position, force, radius);
    
    LOG_INFO("Explosion applied at ({}, {}, {}) with force {} and radius {}", 
            position.x, position.y, position.z, force, radius);
}

void PhysicsLayer::CreatePhysicsDemo()
{
    if (!physicsManager || !sharedWorld) return;
    
    LOG_INFO("Creating physics demo scene...");
    
    // Create some demo objects
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-2.0, 2.0);
    std::uniform_real_distribution<> heightDis(5.0, 15.0);
    
    // Create box stack
    auto boxStack = PhysicsUtils::CreateBoxStack(*physicsManager, 
                                                glm::vec3(-3, 0, 0), 5,
                                                glm::vec3(0.4f, 0.4f, 0.4f));
    
    // Add visuals to box stack
    for (const auto entity : boxStack)
    {
        AddVisualToPhysicsEntity(entity, "box");
    }
    demoEntities.insert(demoEntities.end(), boxStack.begin(), boxStack.end());
    
    // Create ball pit
    auto ballPit = PhysicsUtils::CreateBallPit(*physicsManager,
                                              glm::vec3(3, 8, 0),
                                              glm::vec3(2, 4, 2),
                                              15, 0.3f);
    
    // Add visuals to ball pit
    for (auto entity : ballPit)
    {
        AddVisualToPhysicsEntity(entity, "sphere");
    }
    demoEntities.insert(demoEntities.end(), ballPit.begin(), ballPit.end());
    
    // Create some random objects
    for (int i = 0; i < 10; ++i)
    {
        glm::vec3 pos(dis(gen), heightDis(gen), dis(gen));
        
        if (i % 2 == 0)
        {
            // Box
            auto entity = CreatePhysicsBox(pos, glm::vec3(0.3f, 0.3f, 0.3f), 1.0f);
            demoEntities.push_back(entity);
        }
        else
        {
            // Sphere
            auto entity = CreatePhysicsSphere(pos, 0.4f, 1.0f);
            demoEntities.push_back(entity);
        }
    }
    
    // Create a trigger zone (semi-transparent)
    const auto trigger = physicsManager->CreateBoxEntity(glm::vec3(0, 1, 0), 
                                                  glm::vec3(1.5f, 0.5f, 1.5f), 
                                                  1.0f, true);
    physicsManager->SetTrigger(trigger, true);
    
    // Make trigger zone visible using cube model
    if (cubeEntity != 0 && sharedWorld->HasComponent<ModelComponent>(cubeEntity))
    {
        auto& cubeModelComp = sharedWorld->GetComponent<ModelComponent>(cubeEntity);
        
        ModelComponent modelComp;
        modelComp.ModelData = cubeModelComp.ModelData;
        modelComp.IsLoaded = true;
        modelComp.IsDirty = true;
        modelComp.ModelPath = "primitive://trigger_cube";
        
        sharedWorld->AddComponent(trigger, modelComp);
        sharedWorld->AddComponent(trigger, RenderableComponent(true));
        sharedWorld->AddComponent(trigger, MaterialComponent(glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow
        
        if (sharedWorld->HasComponent<TransformComponent>(trigger))
        {
            auto& transform = sharedWorld->GetComponent<TransformComponent>(trigger);
            transform.scale = glm::vec3(3.0f, 1.0f, 3.0f); // Scale to match trigger size
        }
    }
    
    demoEntities.push_back(trigger);
    
    LOG_INFO("Physics demo created with {} objects", demoEntities.size());
    LOG_INFO("Demo Controls:");
    LOG_INFO("  P: Toggle physics demo");
    LOG_INFO("  Space: Add random objects");
    LOG_INFO("  B: Create explosion at center");
    LOG_INFO("  C: Clear demo objects");
    LOG_INFO("  G/H: Increase/Decrease gravity");
    LOG_INFO("  R: Reset gravity");
}

// Helper function to add visual components to existing physics entities
void PhysicsLayer::AddVisualToPhysicsEntity(const ECS::Entity entity, const std::string& type) const
{
    LOG_DEBUG("=== AddVisualToPhysicsEntity START ===");
    LOG_DEBUG("Entity: {}, Type: {}", entity, type);
    
    if (!modelLayer || !sharedWorld->HasComponent<TransformComponent>(entity))
    {
        LOG_ERROR("ModelLayer is null or entity missing TransformComponent");
        return;
    }
    
    // Add appropriate model based on type
    if (type == "box" || type == "cube")
    {
        if (cubeEntity != 0 && sharedWorld->HasComponent<ModelComponent>(cubeEntity))
        {
            LOG_DEBUG("Adding cube visual to entity {}", entity);
            
            auto& cubeModelComp = sharedWorld->GetComponent<ModelComponent>(cubeEntity);
            
            ModelComponent modelComp;
            modelComp.ModelData = cubeModelComp.ModelData;  // Share mesh data
            modelComp.IsLoaded = true;
            modelComp.IsDirty = true;  // Needs its own buffers
            modelComp.ModelPath = "primitive://cube_util";
            
            sharedWorld->AddComponent(entity, modelComp);
            sharedWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.7f, 0.3f, 0.2f))); // Orange
            
            // Scale to match box collider
            if (sharedWorld->HasComponent<BoxColliderComponent>(entity))
            {
                auto& collider = sharedWorld->GetComponent<BoxColliderComponent>(entity);
                auto& transform = sharedWorld->GetComponent<TransformComponent>(entity);
                transform.scale = collider.size * 2.0f; // Half-extents to full size
                
                LOG_DEBUG("Box entity {} scaled to ({}, {}, {})", 
                         entity, transform.scale.x, transform.scale.y, transform.scale.z);
            }
        }
        else
        {
            LOG_ERROR("Cube entity is invalid or missing ModelComponent!");
        }
    }
    else if (type == "sphere")
    {
        if (sphereEntity != 0 && sharedWorld->HasComponent<ModelComponent>(sphereEntity))
        {
            LOG_DEBUG("Adding sphere visual to entity {}", entity);
            
            auto& sphereModelComp = sharedWorld->GetComponent<ModelComponent>(sphereEntity);
            
            ModelComponent modelComp;
            modelComp.ModelData = sphereModelComp.ModelData;  // Share mesh data
            modelComp.IsLoaded = true;
            modelComp.IsDirty = true;  // Needs its own buffers
            modelComp.ModelPath = "primitive://sphere_util";
            
            sharedWorld->AddComponent(entity, modelComp);
            sharedWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.2f, 0.7f, 0.3f))); // Green
            
            // Scale to match sphere collider
            if (sharedWorld->HasComponent<SphereColliderComponent>(entity))
            {
                auto& collider = sharedWorld->GetComponent<SphereColliderComponent>(entity);
                auto& transform = sharedWorld->GetComponent<TransformComponent>(entity);
                transform.scale = glm::vec3(collider.radius * 2.0f); // Radius to diameter
                
                LOG_DEBUG("Sphere entity {} scaled to ({}, {}, {})", 
                         entity, transform.scale.x, transform.scale.y, transform.scale.z);
            }
        }
        else
        {
            LOG_ERROR("Sphere entity is invalid or missing ModelComponent!");
        }
    }
    
    // Make sure entity is renderable
    if (!sharedWorld->HasComponent<RenderableComponent>(entity))
    {
        sharedWorld->AddComponent(entity, RenderableComponent(true));
    }
    
    LOG_DEBUG("=== AddVisualToPhysicsEntity END ===");
}

void PhysicsLayer::ClearDemo()
{
    if (!sharedWorld) return;
    
    for (const auto entity : demoEntities)
    {
        sharedWorld->DestroyEntity(entity);
    }
    demoEntities.clear();
    
    LOG_INFO("Physics demo cleared");
}

void PhysicsLayer::AddRandomPhysicsObjects()
{
    if (!physicsManager || !sharedWorld) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> posDis(-3.0, 3.0);
    std::uniform_real_distribution<> heightDis(8.0, 12.0);
    std::uniform_int_distribution<> typeDis(0, 1);
    
    glm::vec3 pos(posDis(gen), heightDis(gen), posDis(gen));
    
    if (typeDis(gen) == 0)
    {
        // Add box
        auto entity = CreatePhysicsBox(pos, glm::vec3(0.4f, 0.4f, 0.4f), 1.0f);
        demoEntities.push_back(entity);
    }
	else
	{
        // Add sphere
        auto entity = CreatePhysicsSphere(pos, 0.4f, 1.0f);
        demoEntities.push_back(entity);
    }
}

void PhysicsLayer::HandleDemoInput() const
{
    // Continuous input handling
    if (Input::IsKeyDown(KeyCode::G))
    {
        // Increase gravity
        auto gravity = physicsManager->GetGravity();
        physicsManager->SetGravity(gravity + glm::vec3(0, -0.1f, 0));
    }
    
    if (Input::IsKeyDown(KeyCode::H))
    {
        // Decrease gravity
        auto gravity = physicsManager->GetGravity();
        physicsManager->SetGravity(gravity + glm::vec3(0, 0.1f, 0));
    }
    
    if (Input::IsKeyDown(KeyCode::R))
    {
        // Reset gravity
        physicsManager->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    }
}
