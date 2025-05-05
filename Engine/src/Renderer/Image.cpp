#include "Image.h"
#include "Core.h"

Image::Image(const std::shared_ptr<Device>& device)
	: m_Device(device) {}

Image::Image(const std::shared_ptr<Device>& device, const VkImage image)
	: m_Device(device) , m_Image(image) {}

Image::~Image()
{
	Shutdown();
}

void Image::Shutdown()
{
	if (bIsDestroyed)
		return;
	
	if (m_Device)
	{
		if (m_TextureSampler != VK_NULL_HANDLE)
			vkDestroySampler(m_Device->GetLogicalDevice(), m_TextureSampler, nullptr);

		if (m_ImageView != VK_NULL_HANDLE)
			vkDestroyImageView(m_Device->GetLogicalDevice(), m_ImageView, nullptr);

		if (m_ImageMemory != VK_NULL_HANDLE)
			vkFreeMemory(m_Device->GetLogicalDevice(), m_ImageMemory, nullptr);

		if (m_Image != VK_NULL_HANDLE)
			vkDestroyImage(m_Device->GetLogicalDevice(), m_Image, nullptr);
	}

	bIsDestroyed = true;
}

Image::Image(Image&& other) noexcept: m_Device(other.m_Device)
									, m_Image(other.m_Image)
									, m_ImageView(other.m_ImageView)
									, m_ImageMemory(other.m_ImageMemory)
{
	other.m_Image		= VK_NULL_HANDLE;
	other.m_ImageMemory = VK_NULL_HANDLE;
	other.m_ImageView	= VK_NULL_HANDLE;
}

Image& Image::operator=(Image&& other) noexcept
{
	if (this != &other)
	{
		m_Image				= other.m_Image;
		m_ImageMemory		= other.m_ImageMemory;
		m_ImageView			= other.m_ImageView;
		other.m_Image		= VK_NULL_HANDLE;
		other.m_ImageMemory = VK_NULL_HANDLE;
		other.m_ImageView	= VK_NULL_HANDLE;
	}
	return *this;
}

void Image::CreateImage(
	const VkImageType imageType,
	const uint32_t width,
	const uint32_t height,
	const VkFormat format,
	const VkImageTiling tiling,
	const VkImageUsageFlags usage,
	const VkMemoryPropertyFlags memoryFlags)
{
	// Creation info.
	VkImageCreateInfo image_create_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
	image_create_info.imageType = imageType;
	image_create_info.extent.width = width;
	image_create_info.extent.height = height;
	image_create_info.extent.depth = 1;  // TODO: Support configurable depth.
	image_create_info.mipLevels = 4;     // TODO: Support mip mapping
	image_create_info.arrayLayers = 1;   // TODO: Support number of layers in the image.
	image_create_info.format = format;
	image_create_info.tiling = tiling;
	image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	image_create_info.usage = usage;
	image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;          // TODO: Configurable sample count.
	image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // TODO: Configurable sharing mode.

	if (vkCreateImage(m_Device->GetLogicalDevice(), &image_create_info, nullptr, &m_Image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device->GetLogicalDevice(), m_Image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = m_Device->FindMemoryType(memRequirements.memoryTypeBits, memoryFlags);

	if (vkAllocateMemory(m_Device->GetLogicalDevice(), &allocInfo, nullptr, &m_ImageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	if (vkBindImageMemory(m_Device->GetLogicalDevice(), m_Image, m_ImageMemory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to bind image memory!");
	}
}

void Image::CreateImage(
	const VkImageCreateInfo& imageInfo,
	const VkMemoryPropertyFlags properties,
	VkImage& image,
	VkDeviceMemory& imageMemory) const
{
	if (vkCreateImage(m_Device->GetLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_Device->GetLogicalDevice(), image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = m_Device->FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_Device->GetLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	if (vkBindImageMemory(m_Device->GetLogicalDevice(), image, imageMemory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to bind image memory!");
	}
}

void Image::CreateImageView(const VkFormat format)
{
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = m_Image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // TODO: Make configurable.
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
	
	// TODO: Make configurable
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	if (vkCreateImageView(m_Device->GetLogicalDevice(), &viewInfo, nullptr, &m_ImageView) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create texture image view!");
	}
}