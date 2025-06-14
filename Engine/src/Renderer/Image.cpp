#include "Image.h"

Image::Image(Device* dev, SwapChain* swapChain): device(dev)
{
	CreateImageViews(swapChain);
}

Image::~Image()
{
	for (auto imageView : swapChainImageViews)
	{
		vkDestroyImageView(device->GetLogicalDevice(), imageView, nullptr);
	}
}

void Image::CreateImageViews(const SwapChain* swapChain)
{
	swapChainImageViews.resize(swapChain->GetImages().size());

	for (size_t i = 0; i < swapChain->GetImages().size(); i++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChain->GetImages()[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChain->GetImageFormat();
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(device->GetLogicalDevice(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Image view creation failed!");
		}
		LOG_INFO("Image view created successfully for image index: {}", i);
	}
}
