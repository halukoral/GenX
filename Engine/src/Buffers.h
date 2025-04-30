#pragma once
#include "vulkan/vulkan.h"

struct BufferHandle
{
	VkBuffer Buffer;
	VkDeviceMemory Memory;
};

struct TextureImageHandle
{
	VkImage TextureImage;
	VkDeviceMemory Memory;
};