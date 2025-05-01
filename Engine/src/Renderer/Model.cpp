#include "Model.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#ifndef ENGINE_DIR
#define ENGINE_DIR "../"
#endif

// from: https://stackoverflow.com/a/57595105
template <typename T, typename... Rest>
static void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
{
	seed ^= std::hash<T>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	(hashCombine(seed, rest), ...);
};

namespace std {
	template <>
	struct hash<Model::Vertex>
	{
		size_t operator()(Model::Vertex const &vertex) const
		{
			size_t seed = 0;
			hashCombine(seed, vertex.Position, vertex.Color, vertex.Normal, vertex.TextCoord);
			return seed;
		}
	};
}

void Model::Builder::LoadModel(const std::string& filepath)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filepath.c_str()))
	{
		throw std::runtime_error(warn + err);
	}

	Vertices.clear();
	Indices.clear();

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	for (const auto &shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.Position =
			{
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.TextCoord =
			{
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.Color = {1.0f, 1.0f, 1.0f};

			if (!uniqueVertices.contains(vertex))
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(Vertices.size());
				Vertices.push_back(vertex);
			}
			Indices.push_back(uniqueVertices[vertex]);
		}
	}
}

Model::Model(Device& device, const Model::Builder &builder) : m_Device(device)
{
	CreateVertexBuffers(builder.Vertices);
	CreateIndexBuffers(builder.Indices);
}

Model::~Model()
{
}

std::unique_ptr<Model> Model::CreateModelFromFile(Device& device, const std::string& filepath)
{
	Builder builder{};
	builder.LoadModel(ENGINE_DIR + filepath);
	return std::make_unique<Model>(device, builder);
}

void Model::Bind(const VkCommandBuffer commandBuffer) const
{
	const VkBuffer buffers[] = {m_VertexBuffer->GetBuffer()};
	constexpr VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

	if (m_HasIndexBuffer)
	{
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
	}
}

void Model::Draw(const VkCommandBuffer commandBuffer) const
{
	if (m_HasIndexBuffer)
	{
		vkCmdDrawIndexed(commandBuffer, m_IndexCount, 1, 0, 0, 0);
	}
	else
	{
		vkCmdDraw(commandBuffer, m_VertexCount, 1, 0, 0);
	}
}

void Model::CreateVertexBuffers(const std::vector<Vertex>& vertices)
{
	m_VertexCount = static_cast<uint32_t>(vertices.size());
	assert(m_VertexCount >= 3 && "Vertex count must be at least 3");
	VkDeviceSize bufferSize = sizeof(vertices[0]) * m_VertexCount;
	uint32_t vertexSize = sizeof(vertices[0]);

	Buffer stagingBuffer
	{
		m_Device,
		vertexSize,
		m_VertexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void *)vertices.data());

	m_VertexBuffer = std::make_unique<Buffer>(
		m_Device,
		vertexSize,
		m_VertexCount,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_VertexBuffer->GetBuffer(), bufferSize);
}

void Model::CreateIndexBuffers(const std::vector<uint32_t>& indices)
{
	m_IndexCount = static_cast<uint32_t>(indices.size());
	m_HasIndexBuffer = m_IndexCount > 0;

	if (!m_HasIndexBuffer)
	{
		return;
	}

	const VkDeviceSize bufferSize = sizeof(indices[0]) * m_IndexCount;
	uint32_t indexSize = sizeof(indices[0]);

	Buffer stagingBuffer
	{
		m_Device,
		indexSize,
		m_IndexCount,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
	};

	stagingBuffer.Map();
	stagingBuffer.WriteToBuffer((void *)indices.data());

	m_IndexBuffer = std::make_unique<Buffer>(
		m_Device,
		indexSize,
		m_IndexCount,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	m_Device.CopyBuffer(stagingBuffer.GetBuffer(), m_IndexBuffer->GetBuffer(), bufferSize);
}


std::vector<VkVertexInputBindingDescription> Model::Vertex::GetBindingDescriptions()
{
	std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription> Model::Vertex::GetAttributeDescriptions()
{
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
	attributeDescriptions.push_back({0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Position)});
	attributeDescriptions.push_back({1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)});
	attributeDescriptions.push_back({2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)});
	attributeDescriptions.push_back({3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, TextCoord)});
	return attributeDescriptions;
}