#pragma once
#include "ECS/ECS.h"
#include "ECS/Components/ModelComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Renderer/Device.h"
#include "Renderer/Model.h"
#include <memory>

class PrimitiveModels 
{
private:
    // Singleton for shared primitive data
    struct PrimitiveData {
        std::shared_ptr<Model> cubeModel;
        std::shared_ptr<Model> sphereModel;
        std::shared_ptr<Model> planeModel;
        bool initialized = false;
    };
    
    static PrimitiveData& GetPrimitiveData() {
        static PrimitiveData data;
        return data;
    }
    
public:
    // Initialize all primitive models (call once at startup)
    static void Initialize(Device* device);
    
    // Create entities with shared mesh data
    static ECS::Entity CreateCube(ECS::World* world, float size = 1.0f);
    static ECS::Entity CreateSphere(ECS::World* world, float radius = 1.0f, int segments = 16, int rings = 12);    
    static ECS::Entity CreatePlane(ECS::World* world, float size = 1.0f);
    
private:
    static constexpr float M_PI = 3.14159265358979323846f;
    
    // Mesh generation functions
    static std::shared_ptr<Model> GenerateCubeMesh(float size = 1.0f);
    static std::shared_ptr<Model> GenerateSphereMesh(float radius = 1.0f, int segments = 16, int rings = 12);
    static std::shared_ptr<Model> GeneratePlaneMesh(float size = 1.0f);
    
    // Buffer creation
    static void CreateBuffersForModel(Model* model, Device* device);
    static void CreateMeshBuffers(Mesh& mesh, Device* device);
    static void CreateBufferWithData(VkDeviceSize size, const void* data, VkBufferUsageFlags usage,
                                    VkBuffer& buffer, VkDeviceMemory& bufferMemory, Device* device);
    static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                            VkBuffer& buffer, VkDeviceMemory& bufferMemory, Device* device);
    static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Device* device);
};