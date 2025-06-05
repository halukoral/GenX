// PhysicsLayer.cpp
#include "PhysicsLayer.h"
#include "Application.h"
#include "Event/KeyEvent.h"
#include "Input/Input.h"
#include <random>

PhysicsLayer::PhysicsLayer()
{
    ecsWorld = std::make_unique<ECS::World>();
}

void PhysicsLayer::OnAttach()
{
    LOG_INFO("PhysicsLayer::OnAttach called");
    
    // Get reference to ModelLayer from Application
    auto& app = Application::Get();
    modelLayer = app.GetModelLayer();
    
    if (!modelLayer) {
        LOG_ERROR("ModelLayer not found! Physics objects will not be visible.");
    }
    
    // Create primitive models for physics visualization
    cubeModel = PrimitiveModels::CreateCube(0.5f);
    sphereModel = PrimitiveModels::CreateSphere(0.5f, 12, 8);
    
    LOG_INFO("Created primitive models for physics visualization");
    
    // Create physics manager
    physicsManager = std::make_unique<PhysicsManager>(ecsWorld.get());
    
    // Set up physics world
    physicsManager->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    
    // Create ground plane (invisible, just physics)
    auto ground = physicsManager->CreateGroundPlane(glm::vec3(0, -2, 0));
    physicsManager->SetMaterial(ground, 0.8f, 0.3f); // High friction, some bounce
    
    // Add trigger callback for demo
    physicsManager->AddTriggerCallback([this](const CollisionInfo& collision) {
        LOG_INFO("Trigger collision between entities {} and {}", 
                collision.entityA, collision.entityB);
    });
    
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
    
    if (physicsManager) {
        physicsManager.reset();
    }
    
    if (ecsWorld) {
        ecsWorld.reset();
    }
}

void PhysicsLayer::OnUpdate(float ts)
{
    if (physicsManager) {
        physicsManager->Update(ts);
        
        // Demo controls
        if (demoMode) {
            HandleDemoInput();
        }
    }
}

void PhysicsLayer::OnEvent(Event& event)
{
    EventDispatcher dispatcher(event);
    
    dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& e) {
        if (e.GetKeyCode() == KeyCode::P) {
            // Toggle physics demo
            demoMode = !demoMode;
            if (demoMode) {
                CreatePhysicsDemo();
            } else {
                ClearDemo();
            }
            return true;
        }
        
        if (demoMode) {
            if (e.GetKeyCode() == KeyCode::Space) {
                // Add random objects
                AddRandomPhysicsObjects();
                return true;
            }
            
            if (e.GetKeyCode() == KeyCode::B) {
                // Add explosion at center
                AddExplosion(glm::vec3(0, 5, 0), 15.0f, 8.0f);
                return true;
            }
            
            if (e.GetKeyCode() == KeyCode::C) {
                // Clear demo objects
                ClearDemo();
                return true;
            }
        }
        
        return false;
    });
}

ECS::Entity PhysicsLayer::CreatePhysicsBox(const glm::vec3& position, 
                                          const glm::vec3& size, float mass)
{
    if (!physicsManager) return 0;
    
    auto entity = physicsManager->CreateBoxEntity(position, size, mass);
    physicsManager->SetMaterial(entity, 0.6f, 0.4f); // Medium friction and bounce
    
    // Add visual representation using primitive model
    if (modelLayer && cubeModel) {
        // Create model component with our primitive cube
        ModelComponent modelComp;
        modelComp.modelData = cubeModel;
        modelComp.isLoaded = true;
        ecsWorld->AddComponent(entity, modelComp);
        
        ecsWorld->AddComponent(entity, RenderableComponent(true));
        ecsWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.7f, 0.3f, 0.2f))); // Orange-ish color
        
        // Scale the visual to match physics collider
        if (ecsWorld->HasComponent<TransformComponent>(entity)) {
            auto& transform = ecsWorld->GetComponent<TransformComponent>(entity);
            transform.scale = size * 2.0f; // Physics size is half-extents, visual needs full size
        }
        
        LOG_DEBUG("Created physics box with visual at ({}, {}, {})", position.x, position.y, position.z);
    } else {
        LOG_WARN("ModelLayer or cube model not available - physics box will be invisible");
    }
    
    return entity;
}

ECS::Entity PhysicsLayer::CreatePhysicsSphere(const glm::vec3& position,
                                             float radius, float mass)
{
    if (!physicsManager) return 0;
    
    auto entity = physicsManager->CreateSphereEntity(position, radius, mass);
    physicsManager->SetMaterial(entity, 0.4f, 0.7f); // Low friction, high bounce
    
    // Add visual representation using primitive model
    if (modelLayer && sphereModel) {
        // Create model component with our primitive sphere
        ModelComponent modelComp;
        modelComp.modelData = sphereModel;
        modelComp.isLoaded = true;
        ecsWorld->AddComponent(entity, modelComp);
        
        ecsWorld->AddComponent(entity, RenderableComponent(true));
        ecsWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.2f, 0.7f, 0.3f))); // Green-ish color
        
        // Scale the visual to match physics collider
        if (ecsWorld->HasComponent<TransformComponent>(entity)) {
            auto& transform = ecsWorld->GetComponent<TransformComponent>(entity);
            transform.scale = glm::vec3(radius * 2.0f); // Diameter
        }
        
        LOG_DEBUG("Created physics sphere with visual at ({}, {}, {})", position.x, position.y, position.z);
    } else {
        LOG_WARN("ModelLayer or sphere model not available - physics sphere will be invisible");
    }
    
    return entity;
}

void PhysicsLayer::AddExplosion(const glm::vec3& position, float force, float radius)
{
    if (!physicsManager) return;
    
    PhysicsUtils::ApplyExplosionForce(*physicsManager, demoEntities, position, force, radius);
    
    LOG_INFO("Explosion applied at ({}, {}, {}) with force {} and radius {}", 
            position.x, position.y, position.z, force, radius);
}

void PhysicsLayer::CreatePhysicsDemo()
{
    if (!physicsManager) return;
    
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
    for (auto entity : boxStack) {
        AddVisualToPhysicsEntity(entity, "box");
    }
    demoEntities.insert(demoEntities.end(), boxStack.begin(), boxStack.end());
    
    // Create ball pit
    auto ballPit = PhysicsUtils::CreateBallPit(*physicsManager,
                                              glm::vec3(3, 8, 0),
                                              glm::vec3(2, 4, 2),
                                              15, 0.3f);
    
    // Add visuals to ball pit
    for (auto entity : ballPit) {
        AddVisualToPhysicsEntity(entity, "sphere");
    }
    demoEntities.insert(demoEntities.end(), ballPit.begin(), ballPit.end());
    
    // Create some random objects
    for (int i = 0; i < 10; ++i) {
        glm::vec3 pos(dis(gen), heightDis(gen), dis(gen));
        
        if (i % 2 == 0) {
            // Box
            auto entity = CreatePhysicsBox(pos, glm::vec3(0.3f, 0.3f, 0.3f), 1.0f);
            demoEntities.push_back(entity);
        } else {
            // Sphere
            auto entity = CreatePhysicsSphere(pos, 0.4f, 1.0f);
            demoEntities.push_back(entity);
        }
    }
    
    // Create a trigger zone (semi-transparent)
    auto trigger = physicsManager->CreateBoxEntity(glm::vec3(0, 1, 0), 
                                                  glm::vec3(1.5f, 0.5f, 1.5f), 
                                                  1.0f, true);
    physicsManager->SetTrigger(trigger, true);
    
    // Make trigger zone visible (if you want to see it)
    if (modelLayer && cubeModel) {
        ModelComponent modelComp;
        modelComp.modelData = cubeModel;
        modelComp.isLoaded = true;
        ecsWorld->AddComponent(trigger, modelComp);
        
        ecsWorld->AddComponent(trigger, RenderableComponent(true));
        ecsWorld->AddComponent(trigger, MaterialComponent(glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow trigger zone
        
        if (ecsWorld->HasComponent<TransformComponent>(trigger)) {
            auto& transform = ecsWorld->GetComponent<TransformComponent>(trigger);
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
void PhysicsLayer::AddVisualToPhysicsEntity(ECS::Entity entity, const std::string& type)
{
    if (!modelLayer || !ecsWorld->HasComponent<TransformComponent>(entity)) return;
    
    // Add appropriate model
    if (type == "box" || type == "cube") {
        if (cubeModel) {
            ModelComponent modelComp;
            modelComp.modelData = cubeModel;
            modelComp.isLoaded = true;
            ecsWorld->AddComponent(entity, modelComp);
            
            ecsWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.7f, 0.3f, 0.2f))); // Orange
            
            // Scale to match box collider
            if (ecsWorld->HasComponent<BoxColliderComponent>(entity)) {
                auto& collider = ecsWorld->GetComponent<BoxColliderComponent>(entity);
                auto& transform = ecsWorld->GetComponent<TransformComponent>(entity);
                transform.scale = collider.size * 2.0f; // Half-extents to full size
            }
        }
    }
    else if (type == "sphere") {
        if (sphereModel) {
            ModelComponent modelComp;
            modelComp.modelData = sphereModel;
            modelComp.isLoaded = true;
            ecsWorld->AddComponent(entity, modelComp);
            
            ecsWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.2f, 0.7f, 0.3f))); // Green
            
            // Scale to match sphere collider
            if (ecsWorld->HasComponent<SphereColliderComponent>(entity)) {
                auto& collider = ecsWorld->GetComponent<SphereColliderComponent>(entity);
                auto& transform = ecsWorld->GetComponent<TransformComponent>(entity);
                transform.scale = glm::vec3(collider.radius * 2.0f); // Radius to diameter
            }
        }
    }
    
    ecsWorld->AddComponent(entity, RenderableComponent(true));
}

void PhysicsLayer::ClearDemo()
{
    if (!ecsWorld) return;
    
    for (auto entity : demoEntities) {
        ecsWorld->DestroyEntity(entity);
    }
    demoEntities.clear();
    
    LOG_INFO("Physics demo cleared");
}

void PhysicsLayer::AddRandomPhysicsObjects()
{
    if (!physicsManager) return;
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> posDis(-3.0, 3.0);
    std::uniform_real_distribution<> heightDis(8.0, 12.0);
    std::uniform_int_distribution<> typeDis(0, 1);
    
    glm::vec3 pos(posDis(gen), heightDis(gen), posDis(gen));
    
    if (typeDis(gen) == 0) {
        // Add box
        auto entity = CreatePhysicsBox(pos, glm::vec3(0.4f, 0.4f, 0.4f), 1.0f);
        demoEntities.push_back(entity);
    } else {
        // Add sphere
        auto entity = CreatePhysicsSphere(pos, 0.4f, 1.0f);
        demoEntities.push_back(entity);
    }
}

void PhysicsLayer::HandleDemoInput()
{
    // Continuous input handling
    if (Input::IsKeyDown(KeyCode::G)) {
        // Increase gravity
        auto gravity = physicsManager->GetGravity();
        physicsManager->SetGravity(gravity + glm::vec3(0, -0.1f, 0));
    }
    
    if (Input::IsKeyDown(KeyCode::H)) {
        // Decrease gravity
        auto gravity = physicsManager->GetGravity();
        physicsManager->SetGravity(gravity + glm::vec3(0, 0.1f, 0));
    }
    
    if (Input::IsKeyDown(KeyCode::R)) {
        // Reset gravity
        physicsManager->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    }
}

// Demo Physics Integration Example
/*
UPDATED: No External Models Required!
The physics system now automatically creates primitive models (cubes and spheres) 
for visualization, so you don't need external .obj files.

Usage in your main application:

1. Add PhysicsLayer to your application AFTER ModelLayer:
   - In Application::Init() or Renderer::InitVulkan():
   
   // First ensure ModelLayer exists (should already be there)
   if (!m_ModelLayer) {
       m_ModelLayer = std::make_shared<ModelLayer>();
       Application::Get().PushLayer(m_ModelLayer);
   }
   
   // Then create PhysicsLayer and link to ModelLayer
   auto physicsLayer = std::make_shared<PhysicsLayer>();
   physicsLayer->SetModelLayer(m_ModelLayer.get());
   physicsLayer->EnableDemo(true); // Enable demo mode
   Application::Get().PushLayer(physicsLayer);

2. Demo Controls:
   - P: Toggle physics demo on/off
   - Space: Add random objects (cubes and spheres)
   - B: Create explosion at center (fun!)
   - C: Clear all demo objects
   - G/H: Increase/Decrease gravity
   - R: Reset gravity to normal

3. Creating physics objects in your game:
   
   auto* physics = GetPhysicsLayer()->GetPhysicsManager();
   
   // Create a bouncing ball (automatically rendered as green sphere)
   auto ball = physics->CreateSphereEntity(glm::vec3(0, 10, 0), 0.5f, 1.0f);
   physics->SetMaterial(ball, 0.2f, 0.9f); // Low friction, high bounce
   
   // Create a static wall (automatically rendered as orange box)
   auto wall = physics->CreateBoxEntity(glm::vec3(5, 2, 0), 
                                       glm::vec3(0.1f, 2.0f, 2.0f), 
                                       1.0f, true);
   
   // Apply forces
   physics->AddForce(ball, glm::vec3(100, 0, 0)); // Push right
   physics->AddImpulse(ball, glm::vec3(0, 5, 0)); // Jump

4. Visual Customization:
   Change colors by modifying the MaterialComponent:
   
   // Change color of physics object to red
   if (ecsWorld->HasComponent<MaterialComponent>(entity)) {
       auto& material = ecsWorld->GetComponent<MaterialComponent>(entity);
       material.albedo = glm::vec3(1.0f, 0.0f, 0.0f); // Red color
   }

5. The system automatically creates:
   - Orange boxes for box physics objects
   - Green spheres for sphere physics objects  
   - Yellow boxes for trigger zones
   - Invisible ground planes

6. Performance tip:
   The physics system runs at a fixed 60Hz timestep for stability,
   while rendering runs at your display's refresh rate.

Enjoy experimenting with physics! 🎮
*/