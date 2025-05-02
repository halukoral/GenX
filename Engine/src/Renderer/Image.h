#pragma once
#include "Device.h"

class Image
{
public:
	Image(Device &device, const std::string &filepath);
	~Image();

	Image(const Image &) = delete;
	Image &operator=(const Image &) = delete;
	
	void CreateTextureImage(const std::string& filepath);
	void CreateTextureImageView();
	VkImageView CreateImageView(VkImage image, VkFormat format) const;
	void CreateTextureSampler();

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
					VkDeviceMemory& bufferMemory) const;
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage,
					VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

private:
	Device& m_Device;
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkSampler textureSampler;
	VkImageView textureImageView;
};
