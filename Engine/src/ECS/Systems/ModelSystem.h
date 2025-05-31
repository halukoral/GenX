#pragma once
#include "ECS/ECS.h"
#include "ECS/Components/ModelComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Renderer/Model.h"
#include "Renderer/Device.h"
#include "Renderer/Descriptor.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>
#include <future>

// Forward declarations
class Texture;
class CameraSystem;

// Model Loading System - Handles async model loading
class ModelLoadingSystem : public ECS::System 
{
private:
    ECS::World* world;
    Device* device;
    std::unordered_map<std::string, std::shared_ptr<Model>> modelCache;
    std::unordered_map<std::string, std::future<std::shared_ptr<Model>>> loadingModels;
    
public:
    ModelLoadingSystem() = default;
    
    void SetWorld(ECS::World* w) { world = w; }
    void SetDevice(Device* dev) { device = dev; }
    
    void Update(float dt) override {
        for (auto entity : Entities) {
            auto& modelComp = world->GetComponent<ModelComponent>(entity);
            
            // Start loading if not loaded and not currently loading
            if (!modelComp.isLoaded && !modelComp.modelPath.empty()) {
                StartLoadingModel(entity, modelComp);
            }
            
            // Check if loading completed
            CheckLoadingCompletion(entity, modelComp);
        }
    }
    
    void StartLoadingModel(ECS::Entity entity, ModelComponent& modelComp) {
        const std::string& path = modelComp.modelPath;
        
        // Check cache first
        if (modelCache.contains(path)) {
            modelComp.modelData = modelCache[path];
            modelComp.isLoaded = true;
            CreateModelBuffers(modelComp);
            return;
        }
        
        // Check if already loading
        if (loadingModels.contains(path)) {
            return;
        }
        
        // Start async loading
        loadingModels[path] = std::async(std::launch::async, [path]() {
            try {
                return std::make_shared<Model>(path);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to load model {}: {}", path, e.what());
                return std::shared_ptr<Model>(nullptr);
            }
        });
    }
    
    void CheckLoadingCompletion(ECS::Entity entity, ModelComponent& modelComp) {
        const std::string& path = modelComp.modelPath;
        
        if (loadingModels.contains(path)) {
            auto& future = loadingModels[path];
            
            if (future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                auto model = future.get();
                loadingModels.erase(path);
                
                if (model) {
                    modelCache[path] = model;
                    modelComp.modelData = model;
                    modelComp.isLoaded = true;
                    CreateModelBuffers(modelComp);
                    
                    // Update bounding box if entity has BoundingComponent
                    if (world->HasComponent<BoundingComponent>(entity)) {
                        UpdateBoundingBox(entity, *model);
                    }
                    
                    LOG_INFO("Model loaded successfully: {}", path);
                } else {
                    LOG_ERROR("Failed to load model: {}", path);
                }
            }
        }
    }
    
    void CreateModelBuffers(ModelComponent& modelComp) {
        if (!modelComp.modelData || !device) return;
        
        for (auto& mesh : modelComp.modelData->Meshes) {
            CreateMeshBuffers(mesh);
        }
        modelComp.isDirty = false;
    }
    
    void CreateMeshBuffers(Mesh& mesh) {
        // Vertex buffer
        const VkDeviceSize vertexBufferSize = sizeof(mesh.Vertices[0]) * mesh.Vertices.size();
        CreateBufferWithData(vertexBufferSize, mesh.Vertices.data(),
                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                           mesh.VertexBuffer, mesh.VertexBufferMemory);
        
        // Index buffer
        const VkDeviceSize indexBufferSize = sizeof(mesh.Indices[0]) * mesh.Indices.size();
        CreateBufferWithData(indexBufferSize, mesh.Indices.data(),
                           VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                           mesh.IndexBuffer, mesh.IndexBufferMemory);
    }
    
    void UpdateBoundingBox(ECS::Entity entity, const Model& model) {
        auto& bounds = world->GetComponent<BoundingComponent>(entity);
        
        // Calculate model bounding box
        glm::vec3 minPos(FLT_MAX);
        glm::vec3 maxPos(-FLT_MAX);
        
        for (const auto& mesh : model.Meshes) {
            for (const auto& vertex : mesh.Vertices) {
                minPos = glm::min(minPos, vertex.Pos);
                maxPos = glm::max(maxPos, vertex.Pos);
            }
        }
        
        bounds.center = (minPos + maxPos) * 0.5f;
        bounds.extents = maxPos - minPos;
        bounds.UpdateBounds();
    }
    
private:
    void CreateBufferWithData(VkDeviceSize size, const void* data, VkBufferUsageFlags usage,
                             VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        // Create staging buffer
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    stagingBuffer, stagingBufferMemory);
        
        // Copy data to staging buffer
        void* mapped;
        vkMapMemory(device->GetLogicalDevice(), stagingBufferMemory, 0, size, 0, &mapped);
        memcpy(mapped, data, static_cast<size_t>(size));
        vkUnmapMemory(device->GetLogicalDevice(), stagingBufferMemory);
        
        // Create device buffer
        CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory);
        
        // Copy from staging to device buffer
        CopyBuffer(stagingBuffer, buffer, size);
        
        // Cleanup staging buffer
        vkDestroyBuffer(device->GetLogicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(device->GetLogicalDevice(), stagingBufferMemory, nullptr);
    }
    
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        if (vkCreateBuffer(device->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }
        
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device->GetLogicalDevice(), buffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, properties);
        
        if (vkAllocateMemory(device->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }
        
        vkBindBufferMemory(device->GetLogicalDevice(), buffer, bufferMemory, 0);
    }
    
    void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = device->GetCommandPool();
        allocInfo.commandBufferCount = 1;
        
        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(device->GetLogicalDevice(), &allocInfo, &commandBuffer);
        
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        vkBeginCommandBuffer(commandBuffer, &beginInfo);
        
        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
        
        vkEndCommandBuffer(commandBuffer);
        
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(device->GetGraphicsQueue());
        
        vkFreeCommandBuffers(device->GetLogicalDevice(), device->GetCommandPool(), 1, &commandBuffer);
    }
};

// Model Rendering System - Handles rendering of all models
class ModelRenderSystem : public ECS::System 
{
private:
    ECS::World* world;
    Device* device;
    Descriptor* descriptor;
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    uint32_t currentFrame = 0; // Frame index tracking için
    
    struct RenderCommand {
        ECS::Entity entity;
        glm::mat4 modelMatrix;
        float distance;
    };
    
    std::vector<RenderCommand> renderQueue;
    
public:
    ModelRenderSystem() = default;
    
    void SetWorld(ECS::World* w) { world = w; }
    void SetDevice(Device* dev) { device = dev; }
    void SetDescriptor(Descriptor* desc) { descriptor = desc; }
    void SetPipeline(VkPipeline p, VkPipelineLayout layout) { 
        pipeline = p; 
        pipelineLayout = layout; 
    }
    void SetCurrentFrame(uint32_t frame) { currentFrame = frame; }
    
    void Update(float dt) override {
        // Bu sistem frame update'de değil, render'da çalışır
    }
    
    void Render(VkCommandBuffer commandBuffer, const glm::vec3& cameraPos, 
                const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
        
        if (!pipeline || !pipelineLayout || !descriptor) {
            LOG_ERROR("Pipeline, layout or descriptor is null!");
            return;
        }
        
        // Collect render commands
        CollectRenderCommands(cameraPos);
        
        if (renderQueue.empty()) {
            return; // Render edilecek model yok
        }
        
        LOG_DEBUG("Rendering {} models", renderQueue.size());
        
        // Bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        
        // Her model için ayrı ayrı render et
        for (const auto& cmd : renderQueue) {
            RenderModelWithUniforms(commandBuffer, cmd, viewMatrix, projMatrix);
        }
        
        renderQueue.clear();
    }
    
private:
    void RenderModelWithUniforms(VkCommandBuffer commandBuffer, const RenderCommand& cmd,
                                 const glm::mat4& viewMatrix, const glm::mat4& projMatrix) {
        auto& modelComp = world->GetComponent<ModelComponent>(cmd.entity);
        
        if (!modelComp.modelData) return;
        
        // Uniform buffer'ı bu model için güncelle
        UniformBufferObject ubo{};
        ubo.Model = cmd.modelMatrix;
        ubo.View = viewMatrix;
        ubo.Proj = projMatrix;
        
        // Descriptor'ı güncelle
        descriptor->UpdateUniformBuffer(currentFrame, ubo);
        
        // Descriptor set'i bind et
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                               pipelineLayout, 0, 1, &descriptor->GetDescriptorSet(currentFrame), 
                               0, nullptr);
        
        // Mesh'leri render et
        for (const auto& mesh : modelComp.modelData->Meshes) {
            if (mesh.VertexBuffer == VK_NULL_HANDLE || mesh.IndexBuffer == VK_NULL_HANDLE) {
                continue;
            }
            
            const VkBuffer vertexBuffers[] = {mesh.VertexBuffer};
            const VkDeviceSize offsets[] = {0};
            
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, mesh.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(mesh.Indices.size()), 1, 0, 0, 0);
        }
    }
    
    void CollectRenderCommands(const glm::vec3& cameraPos) {
        renderQueue.clear();
        
        for (auto entity : Entities) {
            auto& modelComp = world->GetComponent<ModelComponent>(entity);
            auto& transform = world->GetComponent<TransformComponent>(entity);
            
            // Skip if not ready
            if (!modelComp.IsReadyForRender()) continue;
            
            // Visibility check
            if (world->HasComponent<RenderableComponent>(entity)) {
                auto& renderable = world->GetComponent<RenderableComponent>(entity);
                if (!renderable.isVisible) continue;
            }
            
            // Create render command
            RenderCommand cmd;
            cmd.entity = entity;
            cmd.modelMatrix = transform.GetTransformMatrix();
            cmd.distance = glm::distance(cameraPos, transform.position);
            
            renderQueue.push_back(cmd);
        }
        
        // Sort by distance (front to back)
        std::sort(renderQueue.begin(), renderQueue.end(), 
                 [](const RenderCommand& a, const RenderCommand& b) {
                     return a.distance < b.distance;
                 });
    }
};