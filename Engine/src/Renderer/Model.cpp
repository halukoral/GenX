#include "Model.h"

// libs
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// Hash function for Vertex3D (for unordered_map)
namespace std
{
	template<> struct hash<glm::vec3>
	{
		size_t operator()(glm::vec3 const& v) const
		{
			return ((hash<float>()(v.x) ^
				   (hash<float>()(v.y) << 1)) >> 1) ^
				   (hash<float>()(v.z) << 1);
		}
	};

	template<> struct hash<glm::vec2>
	{
		size_t operator()(glm::vec2 const& v) const
		{
			return hash<float>()(v.x) ^ (hash<float>()(v.y) << 1);
		}
	};

	template<> struct hash<Vertex3D>
	{
		size_t operator()(Vertex3D const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				   (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				   (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

VkVertexInputBindingDescription Vertex3D::GetBindingDescription()
{
	VkVertexInputBindingDescription bindingDescription{};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex3D);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 4> Vertex3D::GetAttributeDescriptions()
{
	std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

	// Position
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex3D, pos);

	// Normal
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex3D, normal);

	// Texture Coordinates
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex3D, texCoord);

	// Color
	attributeDescriptions[3].binding = 0;
	attributeDescriptions[3].location = 3;
	attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[3].offset = offsetof(Vertex3D, color);

	return attributeDescriptions;
}

void Mesh::Cleanup(const VkDevice device) const
{
	if (indexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, indexBuffer, nullptr);
		vkFreeMemory(device, indexBufferMemory, nullptr);
	}
	if (vertexBuffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
	}
}

Model::Model(const std::string& path)
{
	LoadModel(path);
}

void Model::Cleanup(const VkDevice device) const
{
	for (auto& mesh : m_Meshes)
	{
		mesh.Cleanup(device);
	}
}

glm::mat4 Model::GetModelMatrix() const
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, m_Position);
	model = glm::rotate(model, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	model = glm::rotate(model, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	model = glm::scale(model, m_Scale);
	return model;
}

Model Model::CreateCube()
{
	Model model;
        
	std::vector<Vertex3D> vertices = {
		// Front face
		{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},

		// Back face
		{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {0.5f, 0.5f, 0.5f}},
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {0.8f, 0.2f, 0.6f}},

		// Left face
		{{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.5f, 0.0f}},
		{{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 0.0f}},
		{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.5f, 1.0f}},
		{{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.5f}},

		// Right face
		{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.8f, 0.3f, 0.1f}},
		{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 0.8f, 0.3f}},
		{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.6f, 0.2f, 0.8f}},
		{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.9f, 0.7f, 0.2f}},

		// Bottom face
		{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {0.4f, 0.6f, 0.9f}},
		{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {0.7f, 0.4f, 0.6f}},
		{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.3f, 0.9f, 0.5f}},
		{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.5f, 0.3f}},

		// Top face
		{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.6f, 0.8f, 0.2f}},
		{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 0.6f, 0.8f}},
		{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.9f, 0.3f, 0.7f}},
		{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.5f, 0.9f, 0.4f}}
	};

	std::vector<uint32_t> indices = {
		0,  1,  2,  2,  3,  0,   // front
		4,  5,  6,  6,  7,  4,   // back
		8,  9,  10, 10, 11, 8,   // left
		12, 13, 14, 14, 15, 12,  // right
		16, 17, 18, 18, 19, 16,  // bottom
		20, 21, 22, 22, 23, 20   // top
	};

	model.m_Meshes.emplace_back(vertices, indices);
	return model;
}

Model Model::LoadFromFile(const std::string& path)
{
	Model model;
	model.LoadModel(path);
	return model;
}

void Model::LoadModel(const std::string& path)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
    {
        throw std::runtime_error("TinyObjLoader error: " + warn + err);
    }

    if (!warn.empty())
    {
        std::cout << "TinyObjLoader warning: " << warn << '\n';
    }

    std::unordered_map<Vertex3D, uint32_t> uniqueVertices{};
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex3D vertex{};

            vertex.pos =
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            if (index.normal_index >= 0)
            {
                vertex.normal =
                {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            }
        	else
        	{
                vertex.normal = {0.0f, 0.0f, 1.0f};
            }

            if (index.texcoord_index >= 0)
            {
                vertex.texCoord =
                {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };
            }
        	else
        	{
                vertex.texCoord = {0.0f, 0.0f};
            }

            vertex.color = {0.7f, 0.7f, 0.7f};

            if (!uniqueVertices.contains(vertex))
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }

    CalculateNormals(vertices, indices);

    m_Meshes.emplace_back(vertices, indices);

    std::cout << "Model loaded: " << vertices.size() << " vertices, " 
              << indices.size() / 3 << " triangles" << '\n';
}

void Model::CalculateNormals(std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices)
{
	for (auto& vertex : vertices)
	{
		if (glm::length(vertex.normal) < 0.1f)
		{
			vertex.normal = glm::vec3(0.0f);
		}
	}

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		uint32_t i0 = indices[i];
		uint32_t i1 = indices[i + 1];
		uint32_t i2 = indices[i + 2];

		glm::vec3 v0 = vertices[i0].pos;
		glm::vec3 v1 = vertices[i1].pos;
		glm::vec3 v2 = vertices[i2].pos;

		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

		if (glm::length(vertices[i0].normal) < 0.1f) vertices[i0].normal += faceNormal;
		if (glm::length(vertices[i1].normal) < 0.1f) vertices[i1].normal += faceNormal;
		if (glm::length(vertices[i2].normal) < 0.1f) vertices[i2].normal += faceNormal;
	}

	for (auto& vertex : vertices)
	{
		if (glm::length(vertex.normal) > 0.1f)
		{
			vertex.normal = glm::normalize(vertex.normal);
		}
	}
}
