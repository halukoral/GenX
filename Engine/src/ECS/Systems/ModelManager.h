#pragma once
#include "ECS/ECS.h"
#include "ECS/Components/ModelComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Systems/ModelSystem.h"
#include "Renderer/Device.h"
#include "Renderer/Descriptor.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

class ModelManager 
{
private:
    ECS::World* world;
    Device* device;
    Descriptor* descriptor;
    
    ModelLoadingSystem* loadingSystem;
    ModelRenderSystem* renderSystem;
    
    VkPipeline modelPipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    
public:
    ModelManager(ECS::World* ecsWorld, Device* dev, Descriptor* desc) 
        : world(ecsWorld), device(dev), descriptor(desc) {
        
        // Register systems
        loadingSystem = world->RegisterSystem<ModelLoadingSystem>().get();
        loadingSystem->SetWorld(world);
        loadingSystem->SetDevice(device);
        
        renderSystem = world->RegisterSystem<ModelRenderSystem>().get();
        renderSystem->SetWorld(world);
        renderSystem->SetDevice(device);
        renderSystem->SetDescriptor(descriptor);

    	LOG_INFO("ModelManager - Systems created, setting up signatures...");
    	
        // Set system signatures
        SetupSystemSignatures();

    	LOG_INFO("ModelManager initialization complete");
    }
    
    ~ModelManager() = default;
    
    // Create a simple model entity with transform
    ECS::Entity CreateModelEntity(const std::string& modelPath, 
                                 const glm::vec3& position = glm::vec3(0.0f),
                                 const glm::vec3& rotation = glm::vec3(0.0f),
                                 const glm::vec3& scale = glm::vec3(1.0f)) {
        
        ECS::Entity entity = world->CreateEntity();
        
        // Add transform component
        world->AddComponent(entity, TransformComponent(position, rotation, scale));
        
        // Add model component
        world->AddComponent(entity, ModelComponent(modelPath));
        
        // Add default renderable component
        world->AddComponent(entity, RenderableComponent(true));
        
        // Add default material
        world->AddComponent(entity, MaterialComponent());
        
        // Add bounding component (will be updated when model loads)
        world->AddComponent(entity, BoundingComponent());
        
        return entity;
    }
    
    // Create a model entity with custom material
    ECS::Entity CreateModelEntity(const std::string& modelPath,
                                 const glm::vec3& position,
                                 const MaterialComponent& material) {
        
        ECS::Entity entity = CreateModelEntity(modelPath, position);
        
        // Override default material
        world->GetComponent<MaterialComponent>(entity) = material;
        
        return entity;
    }
    
    // Create multiple instances of the same model
    std::vector<ECS::Entity> CreateModelInstances(const std::string& modelPath,
                                                  const std::vector<glm::vec3>& positions) {
        std::vector<ECS::Entity> entities;
        entities.reserve(positions.size());
        
        for (const auto& pos : positions) {
            entities.push_back(CreateModelEntity(modelPath, pos));
        }
        
        return entities;
    }
    
    // Set model visibility
    void SetModelVisibility(ECS::Entity entity, bool visible) {
        if (world->HasComponent<RenderableComponent>(entity)) {
            world->GetComponent<RenderableComponent>(entity).isVisible = visible;
        }
    }
    
    // Set model material
    void SetModelMaterial(ECS::Entity entity, const MaterialComponent& material) {
        if (world->HasComponent<MaterialComponent>(entity)) {
            world->GetComponent<MaterialComponent>(entity) = material;
        }
    }
    
    // Set model transform
    void SetModelTransform(ECS::Entity entity, const glm::vec3& position, 
                          const glm::vec3& rotation = glm::vec3(0.0f),
                          const glm::vec3& scale = glm::vec3(1.0f)) {
        if (world->HasComponent<TransformComponent>(entity)) {
            auto& transform = world->GetComponent<TransformComponent>(entity);
            transform.position = position;
            transform.SetEulerAngles(rotation);
            transform.scale = scale;
        }
    }
    
    // Get model bounding box in world space
    BoundingComponent GetWorldBounds(ECS::Entity entity) {
        if (!world->HasComponent<BoundingComponent>(entity) || 
            !world->HasComponent<TransformComponent>(entity)) {
            return BoundingComponent();
        }
        
        auto bounds = world->GetComponent<BoundingComponent>(entity);
        auto& transform = world->GetComponent<TransformComponent>(entity);
        
        // Transform bounding box to world space
        glm::mat4 modelMatrix = transform.GetTransformMatrix();
        
        // Transform all 8 corners of the bounding box
        std::vector<glm::vec3> corners = {
            bounds.min,
            glm::vec3(bounds.max.x, bounds.min.y, bounds.min.z),
            glm::vec3(bounds.min.x, bounds.max.y, bounds.min.z),
            glm::vec3(bounds.max.x, bounds.max.y, bounds.min.z),
            glm::vec3(bounds.min.x, bounds.min.y, bounds.max.z),
            glm::vec3(bounds.max.x, bounds.min.y, bounds.max.z),
            glm::vec3(bounds.min.x, bounds.max.y, bounds.max.z),
            bounds.max
        };
        
        glm::vec3 worldMin(FLT_MAX);
        glm::vec3 worldMax(-FLT_MAX);
        
        for (auto& corner : corners) {
            glm::vec4 worldCorner = modelMatrix * glm::vec4(corner, 1.0f);
            glm::vec3 worldPos = glm::vec3(worldCorner);
            worldMin = glm::min(worldMin, worldPos);
            worldMax = glm::max(worldMax, worldPos);
        }
        
        BoundingComponent worldBounds;
        worldBounds.center = (worldMin + worldMax) * 0.5f;
        worldBounds.extents = worldMax - worldMin;
        worldBounds.UpdateBounds();
        
        return worldBounds;
    }
    
    // Check if model is loaded
    bool IsModelLoaded(ECS::Entity entity) {
        if (world->HasComponent<ModelComponent>(entity)) {
            return world->GetComponent<ModelComponent>(entity).isLoaded;
        }
        return false;
    }
    
    // Preload model (useful for loading screens)
    void PreloadModel(const std::string& modelPath) {
        // Create a temporary entity just for loading
        ECS::Entity tempEntity = world->CreateEntity();
        world->AddComponent(tempEntity, ModelComponent(modelPath));
        world->AddComponent(tempEntity, TransformComponent());
        
        // The loading system will handle it, then we can destroy the entity
        // or keep it in cache for later use
    }
    
    // Update systems (call this every frame)
    void Update(float deltaTime) {
        world->Update(deltaTime);
    }
    
    // Render all models (call this during render phase)
	void Render(VkCommandBuffer commandBuffer, 
			   const glm::vec3& cameraPosition,
			   const glm::mat4& viewMatrix, 
			   const glm::mat4& projectionMatrix,
			   uint32_t currentFrame) {  
        
    	if (renderSystem) {
    		renderSystem->SetCurrentFrame(currentFrame); 
    		renderSystem->Render(commandBuffer, cameraPosition, viewMatrix, projectionMatrix);
    	}
    }
    
    // Set render pipeline
    void SetRenderPipeline(VkPipeline pipeline, VkPipelineLayout layout)
	{

    	LOG_INFO("ModelManager::SetRenderPipeline - Pipeline: {}, Layout: {}", 
				 (void*)pipeline, (void*)layout);
    	
    	modelPipeline = pipeline;
        pipelineLayout = layout;
        
        if (renderSystem) {
            renderSystem->SetPipeline(pipeline, layout);
        	LOG_INFO("Pipeline set to render system successfully");
        } else {
        	LOG_ERROR("RenderSystem is null!");
        }
    }

	// Debug metodu ekle
	void DebugState() {
    	LOG_INFO("=== ModelManager Debug ===");
    	LOG_INFO("World: {}", (void*)world);
    	LOG_INFO("Device: {}", (void*)device);
    	LOG_INFO("Descriptor: {}", (void*)descriptor);
    	LOG_INFO("Loading System: {}", (void*)loadingSystem);
    	LOG_INFO("Render System: {}", (void*)renderSystem);
    	LOG_INFO("Model Pipeline: {}", (void*)modelPipeline);
    	LOG_INFO("Pipeline Layout: {}", (void*)pipelineLayout);
    }
    
    // Get statistics
    struct ModelStats {
        int totalEntities = 0;
        int loadedModels = 0;
        int visibleModels = 0;
        int totalTriangles = 0;
    };
    
    ModelStats GetStats() {
        ModelStats stats;
        
        for (auto entity : loadingSystem->Entities) {
            stats.totalEntities++;
            
            auto& modelComp = world->GetComponent<ModelComponent>(entity);
            if (modelComp.isLoaded) {
                stats.loadedModels++;
                
                if (modelComp.modelData) {
                    for (const auto& mesh : modelComp.modelData->Meshes) {
                        stats.totalTriangles += mesh.Indices.size() / 3;
                    }
                }
            }
            
            if (world->HasComponent<RenderableComponent>(entity)) {
                auto& renderable = world->GetComponent<RenderableComponent>(entity);
                if (renderable.isVisible) {
                    stats.visibleModels++;
                }
            }
        }
        
        return stats;
    }
    
    // Remove model entity
    void DestroyModelEntity(ECS::Entity entity) {
        if (world->HasComponent<ModelComponent>(entity)) {
            auto& modelComp = world->GetComponent<ModelComponent>(entity);
            if (modelComp.modelData) {
                // Cleanup GPU resources
                modelComp.modelData->Cleanup(device->GetLogicalDevice());
            }
        }
        world->DestroyEntity(entity);
    }
    
private:
    void SetupSystemSignatures() {
        // Model loading system signature
        ECS::Signature loadingSignature;
        loadingSignature.set(ECS::ComponentTypeCounter::GetTypeId<ModelComponent>());
        loadingSignature.set(ECS::ComponentTypeCounter::GetTypeId<TransformComponent>());
        world->SetSystemSignature<ModelLoadingSystem>(loadingSignature);
        
        // Model render system signature
        ECS::Signature renderSignature;
        renderSignature.set(ECS::ComponentTypeCounter::GetTypeId<ModelComponent>());
        renderSignature.set(ECS::ComponentTypeCounter::GetTypeId<TransformComponent>());
        renderSignature.set(ECS::ComponentTypeCounter::GetTypeId<RenderableComponent>());
        world->SetSystemSignature<ModelRenderSystem>(renderSignature);
    }
};

// Utility functions for creating common model setups
namespace ModelUtils {
    
    // Create a simple cube entity
    inline ECS::Entity CreateCube(ModelManager& manager, const glm::vec3& position = glm::vec3(0.0f)) {
        // This would need a cube model file, or we could generate it procedurally
        return manager.CreateModelEntity("models/cube.obj", position);
    }
    
    // Create a model with PBR material
    inline ECS::Entity CreatePBRModel(ModelManager& manager, 
                                     const std::string& modelPath,
                                     const glm::vec3& position,
                                     const glm::vec3& albedo,
                                     float metallic = 0.0f,
                                     float roughness = 0.5f) {
        MaterialComponent material(albedo, metallic, roughness);
        return manager.CreateModelEntity(modelPath, position, material);
    }
    
    // Create a grid of model instances
    inline std::vector<ECS::Entity> CreateModelGrid(ModelManager& manager,
                                                   const std::string& modelPath,
                                                   int gridX, int gridZ,
                                                   float spacing = 2.0f,
                                                   const glm::vec3& startPos = glm::vec3(0.0f)) {
        std::vector<glm::vec3> positions;
        positions.reserve(gridX * gridZ);
        
        for (int x = 0; x < gridX; ++x) {
            for (int z = 0; z < gridZ; ++z) {
                glm::vec3 pos = startPos + glm::vec3(x * spacing, 0.0f, z * spacing);
                positions.push_back(pos);
            }
        }
        
        return manager.CreateModelInstances(modelPath, positions);
    }
}