#pragma once

#include "ECS/Systems/ModelSystem.h"
#include "ECS/Systems/ModelManager.h"
#include "Layer.h"
#include "Renderer/Device.h"
#include "Renderer/Descriptor.h"
#include <memory>


class ModelLayer : public Layer
{
private:
    std::unique_ptr<ECS::World> ecsWorld;
    std::unique_ptr<ModelManager> modelManager;
    
    // References to external systems
    Device* device = nullptr;
    Descriptor* descriptor = nullptr;
    
public:
    ModelLayer();
    ~ModelLayer() override = default;
    
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnEvent(Event& event) override;
    
    // Public interface for creating models
    ECS::Entity CreateModel(const std::string& modelPath, 
                           const glm::vec3& position = glm::vec3(0.0f),
                           const glm::vec3& rotation = glm::vec3(0.0f),
                           const glm::vec3& scale = glm::vec3(1.0f)) const;
    
    ECS::Entity CreateModelWithMaterial(const std::string& modelPath,
                                       const glm::vec3& position,
                                       const MaterialComponent& material) const;
    
    void SetModelVisibility(ECS::Entity entity, bool visible) const;
    void SetModelTransform(ECS::Entity entity, const glm::vec3& position, 
                          const glm::vec3& rotation = glm::vec3(0.0f),
                          const glm::vec3& scale = glm::vec3(1.0f)) const;
    
    bool IsModelLoaded(ECS::Entity entity) const;
    void DestroyModel(ECS::Entity entity) const;
    
    // Render interface
    void Render(VkCommandBuffer commandBuffer, 
               const glm::vec3& cameraPosition,
               const glm::mat4& viewMatrix, 
               const glm::mat4& projectionMatrix,
			   uint32_t currentFrame) const;
    
    void SetRenderPipeline(VkPipeline pipeline, VkPipelineLayout layout);
    
    // System setup
    void SetDevice(Device* dev) { device = dev; }
    void SetDescriptor(Descriptor* desc) { descriptor = desc; }
    
    // Get stats for debugging
    ModelManager::ModelStats GetModelStats() const;
    
    ModelManager* GetModelManager() const { return modelManager.get(); }

	void DebugState() const;

	VkPipeline storedPipeline = VK_NULL_HANDLE;
	VkPipelineLayout storedPipelineLayout = VK_NULL_HANDLE;
	bool pipelineStored = false;
};
