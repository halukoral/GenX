#pragma once

#include "Buffer.h"
#include "pch.h"
#include "Device.h"
#include "UniformBuffer.h"
#include "Descriptor.h"

struct Vertex3D {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex3D);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex3D, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex3D, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex3D& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex3D> {
        size_t operator()(Vertex3D const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                   (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

class Model {
public:
    Model(Device* device, const std::string& modelPath, VkCommandPool commandPool);
    ~Model();

    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateUniformBuffers();
    void CreateDescriptors();
    
    void UpdateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent);
    void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame);
    
    VkDescriptorSetLayout GetDescriptorSetLayout() const { return descriptor->GetDescriptorSetLayout(); }
    const std::vector<Vertex3D>& GetVertices() const { return vertices; }
    const std::vector<uint32_t>& GetIndices() const { return indices; }

private:
    void LoadModel(const std::string& modelPath);

private:
    Device* device;
    VkCommandPool commandPool;
    
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
    
	// Buffers
	std::unique_ptr<Buffer> vertexBuffer;
	std::unique_ptr<Buffer> indexBuffer;
	std::unique_ptr<UniformBuffer<UniformBufferObject>> uniformBuffers;
    
    // Descriptor management
    std::unique_ptr<Descriptor<UniformBufferObject>> descriptor;
    
    // Transformation matrices
    glm::mat4 modelMatrix;
    glm::mat4 viewMatrix;
    glm::mat4 projMatrix;
    
    static const int MAX_FRAMES_IN_FLIGHT = 2;
};