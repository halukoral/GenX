#pragma once
#include "Device.h"

class Image
{
public:
	Image() = default;
	Image(const std::shared_ptr<Device>& device);
	Image(const std::shared_ptr<Device>& device, VkImage image);
	~Image();

	void Shutdown();
	
	Image(const Image &) = delete;
	Image& operator=(const Image &) = delete;

	// Move ctor
	Image(Image&& other) noexcept;

	// Move assign
	Image& operator=(Image&& other) noexcept;

	void CreateImage(
		const VkImageCreateInfo &imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage &image,
		VkDeviceMemory &imageMemory);
	
	void CreateImageView(VkFormat format);

	[[nodiscard]] VkImage			GetImage() const		{ return m_Image; }
	[[nodiscard]] VkImageView		GetImageView() const	{ return m_ImageView; }
	[[nodiscard]] VkDeviceMemory	GetImageMemory() const	{ return m_ImageMemory; }

	[[nodiscard]] VkImage&			GetImageRef() 			{ return m_Image; }
	[[nodiscard]] VkImageView&		GetImageViewRef() 		{ return m_ImageView; }
	[[nodiscard]] VkDeviceMemory&	GetImageMemoryRef()		{ return m_ImageMemory; }

	void SetDevice(const std::shared_ptr<Device>& device) { m_Device = device; }

private:
	bool bIsDestroyed = false;
	
	std::shared_ptr<Device> m_Device;

	VkImage m_Image {VK_NULL_HANDLE};
	VkImageView m_ImageView {VK_NULL_HANDLE};
	VkDeviceMemory m_ImageMemory {VK_NULL_HANDLE};

	VkSampler m_TextureSampler {VK_NULL_HANDLE};
};
