#include "ModelLayer.h"
#include "Application.h"
#include "Renderer/Renderer.h"

ModelLayer::ModelLayer()
{
    ecsWorld = std::make_unique<ECS::World>();
}

void ModelLayer::OnAttach()
{
	LOG_INFO("ModelLayer::OnAttach called");
    
	// Get references to renderer systems
	auto& app = Application::Get();
	auto renderer = app.GetRenderer();
    
	if (!renderer) {
		LOG_ERROR("Renderer is null!");
		return;
	}
    
	device = renderer->GetDevice();
	descriptor = renderer->GetDescriptor();
    
	LOG_INFO("ModelLayer - Device: {}, Descriptor: {}", (void*)device, (void*)descriptor);
    
	if (!device) {
		LOG_ERROR("Device is null!");
		return;
	}
    
	if (!descriptor) {
		LOG_ERROR("Descriptor is null!");
		return;
	}
    
	// Create model manager
	modelManager = std::make_unique<ModelManager>(ecsWorld.get(), device, descriptor);
	if (pipelineStored)
	{
		modelManager->SetRenderPipeline(storedPipeline, storedPipelineLayout);
	}
	
	LOG_INFO("ModelLayer attached successfully - calling DebugState");
	DebugState();
}

void ModelLayer::OnDetach()
{
    LOG_INFO("ModelLayer detached");
    
    if (modelManager) {
        // Cleanup will happen automatically through destructors
        modelManager.reset();
    }
    
    if (ecsWorld) {
        ecsWorld.reset();
    }
}

void ModelLayer::OnUpdate(float ts)
{
    if (modelManager) {
        modelManager->Update(ts);
    }
}

void ModelLayer::OnEvent(Event& event)
{
    // Handle model-specific events if needed
    // For example, model visibility toggles, LOD changes, etc.
}

ECS::Entity ModelLayer::CreateModel(const std::string& modelPath, 
                                   const glm::vec3& position,
                                   const glm::vec3& rotation,
                                   const glm::vec3& scale)
{
    if (!modelManager)
    {
        LOG_ERROR("ModelManager not initialized!");
        return 0;
    }
    
    ECS::Entity entity = modelManager->CreateModelEntity(modelPath, position, rotation, scale);
    LOG_INFO("Created model entity {} with path: {}", entity, modelPath);
    return entity;
}

ECS::Entity ModelLayer::CreateModelWithMaterial(const std::string& modelPath,
                                               const glm::vec3& position,
                                               const MaterialComponent& material)
{
    if (!modelManager)
    {
        LOG_ERROR("ModelManager not initialized!");
        return 0;
    }
    
    return modelManager->CreateModelEntity(modelPath, position, material);
}

void ModelLayer::SetModelVisibility(ECS::Entity entity, bool visible)
{
    if (modelManager) {
        modelManager->SetModelVisibility(entity, visible);
    }
}

void ModelLayer::SetModelTransform(ECS::Entity entity, const glm::vec3& position, 
                                  const glm::vec3& rotation, const glm::vec3& scale)
{
    if (modelManager) {
        modelManager->SetModelTransform(entity, position, rotation, scale);
    }
}

bool ModelLayer::IsModelLoaded(ECS::Entity entity)
{
    if (modelManager) {
        return modelManager->IsModelLoaded(entity);
    }
    return false;
}

void ModelLayer::DestroyModel(ECS::Entity entity)
{
    if (modelManager)
    {
        modelManager->DestroyModelEntity(entity);
    }
}

void ModelLayer::Render(VkCommandBuffer commandBuffer, 
                       const glm::vec3& cameraPosition,
                       const glm::mat4& viewMatrix, 
                       const glm::mat4& projectionMatrix,
					   uint32_t currentFrame)
{
    if (modelManager)
    {
        modelManager->Render(commandBuffer, cameraPosition, viewMatrix, projectionMatrix, currentFrame);
    }
}

void ModelLayer::SetRenderPipeline(VkPipeline pipeline, VkPipelineLayout layout)
{
	LOG_INFO("ModelLayer::SetRenderPipeline called - Pipeline: {}, Layout: {}", 
		 (void*)pipeline, (void*)layout);
	
    if (modelManager)
    {
        modelManager->SetRenderPipeline(pipeline, layout);
    } else {
    	LOG_ERROR("ModelManager is null in SetRenderPipeline!");
    	storedPipeline = pipeline;
    	storedPipelineLayout = layout;
    	pipelineStored = true;
    }
}

ModelManager::ModelStats ModelLayer::GetModelStats()
{
    if (modelManager)
    {
        return modelManager->GetStats();
    }
    return ModelManager::ModelStats{};
}

void ModelLayer::DebugState() const
{
	LOG_INFO("=== ModelLayer Debug ===");
	LOG_INFO("Device: {}", (void*)device);
	LOG_INFO("Descriptor: {}", (void*)descriptor);
	LOG_INFO("ModelManager: {}", (void*)modelManager.get());
        
	if (modelManager) {
		modelManager->DebugState();
	}
}
