// PhysicsLayer.cpp
#include "PhysicsLayer.h"
#include "Application.h"
#include "Event/KeyEvent.h"
#include "Input/Input.h"
#include <random>

void PhysicsLayer::OnAttach()
{
    LOG_INFO("PhysicsLayer::OnAttach called");
    
    // Get reference to ModelLayer from Application
    const auto& app = Application::Get();
    modelLayer = app.GetModelLayer();
    
    if (!modelLayer)
    {
        LOG_ERROR("ModelLayer not found! Physics system cannot work.");
        return;
    }
    
    LOG_INFO("ModelLayer found: {}", (void*)modelLayer);
    
    sharedWorld = modelLayer->GetModelManager()->GetWorld();
    
    if (!sharedWorld)
    {
        LOG_ERROR("Shared ECS World not found!");
        return;
    }
    
    LOG_INFO("Shared world found: {}", (void*)sharedWorld);

    const auto renderer = app.GetRenderer();
    if (!renderer)
    {
        LOG_ERROR("Renderer not found!");
        return;
    }
    
    Device* device = renderer->GetDevice();
    if (!device)
    {
        LOG_ERROR("Device not found!");
        return;
    }
    
    LOG_INFO("Device found: {}", (void*)device);
    
    // Create primitive models for physics visualization WITH DEVICE
    LOG_INFO("Creating primitive models with device...");
    
    try
    {
    	auto Model = modelLayer->CreateModel("../cube.obj", glm::vec3(0, 0, 2));
        cubeModel = PrimitiveModels::CreateCube(0.5f, device);
        LOG_INFO("Cube model created - Meshes: {}", cubeModel ? cubeModel->Meshes.size() : 0);
        
        if (cubeModel && !cubeModel->Meshes.empty())
        {
            const auto& mesh = cubeModel->Meshes[0];
            LOG_INFO("Cube mesh - Vertices: {}, Indices: {}, VB: {}, IB: {}", 
                    mesh.Vertices.size(), mesh.Indices.size(), 
                    (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
        }
        
        sphereModel = PrimitiveModels::CreateSphere(0.5f, 32, 12, device);
        LOG_INFO("Sphere model created - Meshes: {}", sphereModel ? sphereModel->Meshes.size() : 0);
        
        if (sphereModel && !sphereModel->Meshes.empty())
        {
            const auto& mesh = sphereModel->Meshes[0];
            LOG_INFO("Sphere mesh - Vertices: {}, Indices: {}, VB: {}, IB: {}", 
                    mesh.Vertices.size(), mesh.Indices.size(), 
                    (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
        }
        
    }
	catch (const std::exception& e)
    {
        LOG_ERROR("Failed to create primitive models: {}", e.what());
        return;
    }
    
    LOG_INFO("Created primitive models for physics visualization");
    
    // Create physics manager with SHARED world
    physicsManager = std::make_unique<PhysicsManager>(sharedWorld);
    
    // Set up physics world
	//physicsManager->SetGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    physicsManager->SetGravity(glm::vec3(0.0f, 0.0f, 0.0f));
    
    // Create ground plane (invisible, just physics)
    auto ground = physicsManager->CreateGroundPlane(glm::vec3(0, -2, 0));
    physicsManager->SetMaterial(ground, 0.8f, 0.3f); // High friction, some bounce
    
    // Create demo scene if enabled
    if (demoMode)
    {
        CreatePhysicsDemo();
    }
    
    LOG_INFO("PhysicsLayer attached successfully");
}

void PhysicsLayer::OnDetach()
{
    LOG_INFO("PhysicsLayer detached");
    
    ClearDemo();
    
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
    LOG_DEBUG("Position: ({}, {}, {}), Size: ({}, {}, {}), Mass: {}", 
             position.x, position.y, position.z, size.x, size.y, size.z, mass);
    
    if (!physicsManager || !sharedWorld)
    {
        LOG_ERROR("PhysicsManager or SharedWorld is null!");
        return 0;
    }
    
    auto entity = physicsManager->CreateBoxEntity(position, size, mass);
    physicsManager->SetMaterial(entity, 0.6f, 0.4f); // Medium friction and bounce
    
    LOG_DEBUG("Physics entity created: {}", entity);
    
    // Add visual representation using primitive model
    if (modelLayer && cubeModel)
    {
        LOG_DEBUG("Adding visual representation to entity {}", entity);
        
        // Buffer kontrolü ekle
        if (!cubeModel->Meshes.empty())
        {
            const auto& mesh = cubeModel->Meshes[0];
            if (mesh.VertexBuffer == VK_NULL_HANDLE || mesh.IndexBuffer == VK_NULL_HANDLE)
            {
                LOG_ERROR("Cube model buffers are null! VB: {}, IB: {}", 
                         (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
                return entity; // Physics entity'yi döndür ama visual ekleme
            }
            
            LOG_DEBUG("Cube buffers OK - VB: {}, IB: {}", 
                     (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
        }
        
        // Create model component with our primitive cube
        ModelComponent modelComp;
        modelComp.modelData = cubeModel;
        modelComp.isLoaded = true;
        modelComp.modelPath = "primitive_cube"; // For debugging
        
        LOG_DEBUG("Adding ModelComponent to entity {}", entity);
        sharedWorld->AddComponent(entity, modelComp);
        
        LOG_DEBUG("Adding RenderableComponent to entity {}", entity);
        sharedWorld->AddComponent(entity, RenderableComponent(true));
        
        LOG_DEBUG("Adding MaterialComponent to entity {}", entity);
        sharedWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.7f, 0.3f, 0.2f))); // Orange-ish color
        
        // Scale the visual to match physics collider
        if (sharedWorld->HasComponent<TransformComponent>(entity)) {
            auto& transform = sharedWorld->GetComponent<TransformComponent>(entity);
            transform.scale = size * 2.0f; // Physics size is half-extents, visual needs full size
            
            LOG_DEBUG("Entity {} transform - Pos: ({}, {}, {}), Scale: ({}, {}, {})", 
                     entity, transform.position.x, transform.position.y, transform.position.z,
                     transform.scale.x, transform.scale.y, transform.scale.z);
        }
        
        LOG_INFO("Created physics box with visual at ({}, {}, {})", position.x, position.y, position.z);
    }
    else
    {
        LOG_ERROR("ModelLayer or cube model not available - modelLayer={}, cubeModel={}", 
                 (void*)modelLayer, (void*)cubeModel.get());
    }

    LOG_DEBUG("=== CreatePhysicsBox END ===");
    return entity;
}

ECS::Entity PhysicsLayer::CreatePhysicsSphere(const glm::vec3& position,float radius, float mass) const
{
    LOG_DEBUG("=== CreatePhysicsSphere START ===");
    LOG_DEBUG("Position: ({}, {}, {}), Radius: {}, Mass: {}", 
             position.x, position.y, position.z, radius, mass);
    
    if (!physicsManager || !sharedWorld)
    {
        LOG_ERROR("PhysicsManager or SharedWorld is null!");
        return 0;
    }
    
    auto entity = physicsManager->CreateSphereEntity(position, radius, mass);
    physicsManager->SetMaterial(entity, 0.4f, 0.7f); // Low friction, high bounce
    
    LOG_DEBUG("Physics entity created: {}", entity);
    
    // Add visual representation using primitive model
    if (modelLayer && sphereModel) {
        LOG_DEBUG("Adding visual representation to entity {}", entity);
        
        // Buffer kontrolü ekle
        if (!sphereModel->Meshes.empty())
        {
            const auto& mesh = sphereModel->Meshes[0];
            if (mesh.VertexBuffer == VK_NULL_HANDLE || mesh.IndexBuffer == VK_NULL_HANDLE)
            {
                LOG_ERROR("Sphere model buffers are null! VB: {}, IB: {}", 
                         (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
                return entity; // Physics entity'yi döndür ama visual ekleme
            }
            
            LOG_DEBUG("Sphere buffers OK - VB: {}, IB: {}", 
                     (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
        }
        
        // Create model component with our primitive sphere
        ModelComponent modelComp;
        modelComp.modelData = sphereModel;
        modelComp.isLoaded = true;
        modelComp.modelPath = "primitive_sphere"; // For debugging
        
        LOG_DEBUG("Adding ModelComponent to entity {}", entity);
        sharedWorld->AddComponent(entity, modelComp);
        
        LOG_DEBUG("Adding RenderableComponent to entity {}", entity);
        sharedWorld->AddComponent(entity, RenderableComponent(true));
        
        LOG_DEBUG("Adding MaterialComponent to entity {}", entity);
        sharedWorld->AddComponent(entity, MaterialComponent(glm::vec3(0.2f, 0.7f, 0.3f))); // Green-ish color
        
        // Scale the visual to match physics collider
        if (sharedWorld->HasComponent<TransformComponent>(entity)) {
            auto& transform = sharedWorld->GetComponent<TransformComponent>(entity);
            transform.scale = glm::vec3(radius * 2.0f); // Diameter
            
            LOG_DEBUG("Entity {} transform - Pos: ({}, {}, {}), Scale: ({}, {}, {})", 
                     entity, transform.position.x, transform.position.y, transform.position.z,
                     transform.scale.x, transform.scale.y, transform.scale.z);
        }
        
        LOG_INFO("Created physics sphere with visual at ({}, {}, {})", position.x, position.y, position.z);
    } else {
        LOG_ERROR("ModelLayer or sphere model not available - modelLayer={}, sphereModel={}", 
                 (void*)modelLayer, (void*)sphereModel.get());
    }
    
    LOG_DEBUG("=== CreatePhysicsSphere END ===");
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
    
    // Make trigger zone visible (if you want to see it)
    if (modelLayer && cubeModel)
    {
        ModelComponent modelComp;
        modelComp.modelData = cubeModel;
        modelComp.isLoaded = true;
        sharedWorld->AddComponent(trigger, modelComp);
        
        sharedWorld->AddComponent(trigger, RenderableComponent(true));
        sharedWorld->AddComponent(trigger, MaterialComponent(glm::vec3(1.0f, 1.0f, 0.0f))); // Yellow trigger zone
        
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
    
    // Add appropriate model
    if (type == "box" || type == "cube")
    {
        if (cubeModel)
        {
            LOG_DEBUG("Adding cube visual to entity {}", entity);
            
            ModelComponent modelComp;
            modelComp.modelData = cubeModel;
            modelComp.isLoaded = true;
            modelComp.modelPath = "primitive_cube_util"; // For debugging
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
            LOG_ERROR("Cube model is null!");
        }
    }
    else if (type == "sphere")
    {
        if (sphereModel)
        {
            LOG_DEBUG("Adding sphere visual to entity {}", entity);
            
            ModelComponent modelComp;
            modelComp.modelData = sphereModel;
            modelComp.isLoaded = true;
            modelComp.modelPath = "primitive_sphere_util"; // For debugging
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
            LOG_ERROR("Sphere model is null!");
        }
    }
    
    sharedWorld->AddComponent(entity, RenderableComponent(true));
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
