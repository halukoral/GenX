#pragma once

#include "pch.h"
#include <glm/glm.hpp>

struct Vertex
{
	Vertex() : Position(glm::vec3(0.0f)), TextCoord(glm::vec2(0.0f)) {}
	Vertex(const glm::vec3 position, const glm::vec2 textCoord) : Position(position), TextCoord(textCoord) {}
	
	glm::vec3 Position;
	glm::vec2 TextCoord;

	bool operator==(const Vertex& other) const
	{
		return Position == other.Position && TextCoord == other.TextCoord;
	}
	
	static VkVertexInputBindingDescription GetBindingDescription()
	{
		VkVertexInputBindingDescription description = {};
		description.binding = 0;
		description.stride = sizeof(Vertex);
		description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return description;
	}

	static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions()
	{
		std::array<VkVertexInputAttributeDescription, 2> descriptions = {};

		descriptions[0].binding = 0;
		descriptions[0].location = 0;
		descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		descriptions[0].offset = offsetof(Vertex, Position);

		descriptions[1].binding = 0;
		descriptions[1].location = 1;
		descriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		descriptions[1].offset = offsetof(Vertex, TextCoord);

		return descriptions;
	}
};
