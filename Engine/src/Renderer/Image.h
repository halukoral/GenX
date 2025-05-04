#pragma once
#include "Device.h"

class Image
{
public:
	Image() {}
	Image(const std::shared_ptr<Device>& device);
	Image(const std::shared_ptr<Device>& device, VkImage image);
	Image(const std::shared_ptr<Device>& device, const std::string &filepath);
	~Image();

	void Shutdown();
	
	Image(const Image &) = delete;
	Image& operator=(const Image &) = delete;

	// Move ctor
	Image(Image&& other) noexcept
	  : m_Device(other.m_Device)
	  , m_Image(other.m_Image)
	  , m_ImageMemory(other.m_ImageMemory)
	  , m_ImageView(other.m_ImageView)
	{
		other.m_Image      = VK_NULL_HANDLE;
		other.m_ImageMemory = VK_NULL_HANDLE;
		other.m_ImageView  = VK_NULL_HANDLE;
	}

	// Move assign
	Image& operator=(Image&& other) noexcept
	{
		if (this != &other)
		{
			m_Image       = other.m_Image;
			m_ImageMemory  = other.m_ImageMemory;
			m_ImageView   = other.m_ImageView;
			other.m_Image      = VK_NULL_HANDLE;
			other.m_ImageMemory = VK_NULL_HANDLE;
			other.m_ImageView  = VK_NULL_HANDLE;
		}
		return *this;
	}
	
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
	std::shared_ptr<Device> m_Device;

	VkImage m_Image {VK_NULL_HANDLE};
	VkImageView m_ImageView {VK_NULL_HANDLE};
	VkDeviceMemory m_ImageMemory {VK_NULL_HANDLE};

	VkSampler m_TextureSampler {VK_NULL_HANDLE};
};
