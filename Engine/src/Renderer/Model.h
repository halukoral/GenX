#pragma once

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "Buffer.h"
#include "Device.h"


class Model
{
public:
	struct Vertex
	{
		glm::vec3 Position{};
		glm::vec3 Color{};
		glm::vec3 Normal{};
		glm::vec2 TextCoord{};
		
		static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
		static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

		bool operator==(const Vertex &other) const
		{
			return	Position == other.Position &&
					Color == other.Color &&
					Normal == other.Normal &&
					TextCoord == other.TextCoord;
		}
	};

	struct Builder
	{
		std::vector<Vertex> Vertices{};
		std::vector<uint32_t> Indices{};

		void LoadModel(const std::string &filepath);
	};
	
	Model(Device &device, const Model::Builder &builder);
	~Model();

	Model(const Model &) = delete;
	Model &operator=(const Model &) = delete;

	static std::unique_ptr<Model> CreateModelFromFile(Device &device, const std::string &filepath);
	
	void Bind(VkCommandBuffer commandBuffer) const;
	void Draw(VkCommandBuffer commandBuffer) const;

private:
	void CreateVertexBuffers(const std::vector<Vertex> &vertices);
	void CreateIndexBuffers(const std::vector<uint32_t> &indices);
	
	Device &m_Device;
	
	std::unique_ptr<Buffer> m_VertexBuffer;
	uint32_t m_VertexCount;

	bool m_HasIndexBuffer = false;
	std::unique_ptr<Buffer> m_IndexBuffer;
	uint32_t m_IndexCount;
};
