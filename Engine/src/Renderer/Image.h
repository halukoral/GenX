#pragma once

#include "pch.h"
#include "Device.h"
#include "SwapChain.h"

class Image
{
public:
	Image(Device* dev, SwapChain* swapChain);
	~Image();

	const std::vector<VkImageView>& GetImageViews() const { return swapChainImageViews; }

private:
	void CreateImageViews(SwapChain* swapChain);

private:
	std::vector<VkImageView> swapChainImageViews;
	Device* device;
};