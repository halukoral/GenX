#pragma once
#include "Device.h"

class Image
{
public:
	Image(Device &device);
	Image(Device &device, VkImage& image);
	Image(Device &device, const std::string &filepath);
	~Image();

	void CreateTextureImage(const std::string& filepath);
	void CreateTextureImageView();
	void CreateTextureSampler();

	void CreateImage(
		uint32_t width,
		uint32_t height,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory) const;
	
	void CreateImageView(VkFormat format);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
					VkDeviceMemory& bufferMemory) const;

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) const;
	void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

	[[nodiscard]] VkImage			GetImage() const		{ return m_Image; }
	[[nodiscard]] VkImageView		GetImageView() const	{ return m_ImageView; }
	[[nodiscard]] VkDeviceMemory	GetImageMemory() const	{ return m_ImageMemory; }

	[[nodiscard]] VkImage&			GetImageRef() 			{ return m_Image; }
	[[nodiscard]] VkImageView&		GetImageViewRef() 		{ return m_ImageView; }
	[[nodiscard]] VkDeviceMemory&	GetImageMemoryRef()		{ return m_ImageMemory; }

	void SetImage(const VkImage& image) { m_Image = image; }
	
private:
	Device& m_Device;

	VkImage m_Image;
	VkImageView m_ImageView;
	VkDeviceMemory m_ImageMemory;

	VkSampler m_TextureSampler;
};
