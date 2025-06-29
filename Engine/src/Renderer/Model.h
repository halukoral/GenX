#pragma once

#include "Buffer.h"
#include "pch.h"
#include "Device.h"
#include "UniformBuffer.h"
#include "Descriptor.h"

struct Vertex3D
{
    glm::vec3 Pos;
    glm::vec3 Color;
    glm::vec2 TexCoord;

    static VkVertexInputBindingDescription GetBindingDescription()
	{
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex3D);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
	{
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex3D, Pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex3D, Color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex3D, TexCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex3D& other) const
	{
        return Pos == other.Pos && Color == other.Color && TexCoord == other.TexCoord;
    }
};

namespace std
{
    template<> struct hash<Vertex3D>
	{
        size_t operator()(Vertex3D const& vertex) const
    	{
            return ((hash<glm::vec3>()(vertex.Pos) ^
                   (hash<glm::vec3>()(vertex.Color) << 1)) >> 1) ^
                   (hash<glm::vec2>()(vertex.TexCoord) << 1);
        }
    };
}

struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};

class Model
{
public:
	Model(Device* device, const std::string& modelPath, VkCommandPool commandPool);
	~Model();

	void CreateVertexBuffer();
	void CreateIndexBuffer();
	void CreateUniformBuffers();
	void CreateDescriptors();

	void UpdateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent) const;
	void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame);

	VkDescriptorSetLayout GetDescriptorSetLayout() const { return m_Descriptor->GetDescriptorSetLayout(); }
	const std::vector<Vertex3D>& GetVertices() const { return m_Vertices; }
	const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

private:
	void LoadModel(const std::string& modelPath);

private:
	Device* m_Device;
	VkCommandPool m_CommandPool;

	std::vector<Vertex3D> m_Vertices;
	std::vector<uint32_t> m_Indices;

	// Buffers
	std::unique_ptr<Buffer> m_VertexBuffer;
	std::unique_ptr<Buffer> m_IndexBuffer;
	std::unique_ptr<UniformBuffer<UniformBufferObject>> m_UniformBuffers;

	// Descriptor management
	std::unique_ptr<Descriptor<UniformBufferObject>> m_Descriptor;

	// Transformation matrices
	glm::mat4 m_ModelMatrix;
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjMatrix;

	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
};