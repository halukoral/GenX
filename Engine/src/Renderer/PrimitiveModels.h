#pragma once
#include "Renderer/Model.h"
#include "Renderer/Device.h"
#include <memory>

class PrimitiveModels 
{
public:
    // Create basic geometric shapes for physics visualization
    static std::shared_ptr<Model> CreateCube(float size = 1.0f, Device* device = nullptr);
	static std::shared_ptr<Model> CreateSphere(float radius = 1.0f, int segments = 16, int rings = 12, Device* device = nullptr);    
    static std::shared_ptr<Model> CreatePlane(float size = 1.0f, Device* device = nullptr);

private:
    static constexpr float M_PI = 3.14159265358979323846f;
    
    static void CreateBuffersForModel(Model* model, Device* device);

	static void CreateMeshBuffers(Mesh& mesh, Device* device);

	static void CreateBufferWithData(VkDeviceSize size, const void* data, VkBufferUsageFlags usage,
									VkBuffer& buffer, VkDeviceMemory& bufferMemory, Device* device);

	static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
							VkBuffer& buffer, VkDeviceMemory& bufferMemory, Device* device);

	static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Device* device);
};