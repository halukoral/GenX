#pragma once

#include "pch.h"
#include "Device.h"

class Texture
{
public:
	Texture(Device* device, const std::string& imagePath);
	~Texture();

	VkImageView GetImageView() const { return m_TextureImageView; }
	VkSampler GetSampler() const { return m_TextureSampler; }

private:
	void CreateTextureImage(const std::string& imagePath);
	void CreateTextureImageView();
	void CreateTextureSampler();
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
					VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
					VkImage& image, VkDeviceMemory& imageMemory);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
					VkDeviceMemory& bufferMemory);

private:
	Device* m_Device;
	VkImage m_TextureImage;
	VkDeviceMemory m_TextureImageMemory;
	VkImageView m_TextureImageView;
	VkSampler m_TextureSampler;
};