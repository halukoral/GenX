#include "Model.h"
#include <unordered_map>
#include <chrono>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

Model::Model(Device* device, const std::string& modelPath, VkCommandPool commandPool) 
    : m_Device(device), m_CommandPool(commandPool)
{
    LoadModel(modelPath);
    
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateUniformBuffers();
    CreateDescriptors();
    
    // Initialize transformation matrices
    m_ModelMatrix = glm::mat4(1.0f);
    m_ViewMatrix = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
}

Model::~Model() = default;

void Model::LoadModel(const std::string& modelPath)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
    {
        throw std::runtime_error("Failed to load model: " + warn + err);
    }

    std::unordered_map<Vertex3D, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex3D vertex{};

            vertex.Pos =
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.texcoord_index >= 0)
            {
                vertex.TexCoord =
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
        	else
        	{
                vertex.TexCoord = {0.0f, 0.0f};
            }

            vertex.Color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(m_Vertices.size());
                m_Vertices.push_back(vertex);
            }

            m_Indices.push_back(uniqueVertices[vertex]);
        }
    }

    LOG_INFO("Model loaded: {} vertices, {} indices", m_Vertices.size(), m_Indices.size());
}

void Model::CreateVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

	// Create staging buffer
	Buffer stagingBuffer(
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_CommandPool
	);

	// Copy vertex data to staging buffer
	stagingBuffer.WriteToBuffer(m_Vertices.data(), bufferSize);

	// Create vertex buffer
	m_VertexBuffer = std::make_unique<Buffer>(
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_CommandPool
	);

	// Copy from staging to vertex buffer
	m_VertexBuffer->CopyFrom(stagingBuffer, bufferSize);
}

void Model::CreateIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(m_Indices[0]) * m_Indices.size();

	// Create staging buffer
	Buffer stagingBuffer(
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_CommandPool
	);

	// Copy index data to staging buffer
	stagingBuffer.WriteToBuffer(m_Indices.data(), bufferSize);

	// Create index buffer
	m_IndexBuffer = std::make_unique<Buffer>(
		m_Device,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_CommandPool
	);

	// Copy from staging to index buffer
	m_IndexBuffer->CopyFrom(stagingBuffer, bufferSize);
}

void Model::CreateUniformBuffers()
{
	m_UniformBuffers = std::make_unique<UniformBuffer<UniformBufferObject>>(m_Device, MAX_FRAMES_IN_FLIGHT);
}

void Model::CreateDescriptors()
{
    m_Descriptor = std::make_unique<Descriptor<UniformBufferObject>>(m_Device, MAX_FRAMES_IN_FLIGHT);
    
    m_Descriptor->CreateDescriptorSetLayout();
    m_Descriptor->CreateDescriptorPool();
    m_Descriptor->CreateDescriptorSets(m_UniformBuffers.get());
}

void Model::UpdateUniformBuffer(uint32_t currentImage, VkExtent2D swapChainExtent) const
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	const auto currentTime = std::chrono::high_resolution_clock::now();
	const float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo{};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = m_ViewMatrix;
	ubo.proj = glm::perspective(glm::radians(75.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1; // GLM was designed for OpenGL, where the Y coordinate is inverted

	m_UniformBuffers->UpdateUniform(ubo, currentImage);
}

void Model::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t currentFrame)
{
    VkBuffer vertexBuffers[] = {m_VertexBuffer->GetBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
    
    VkDescriptorSet descriptorSet = m_Descriptor->GetDescriptorSet(currentFrame);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
}