#pragma once

#include "pch.h"
#include "Device.h"
#include "SwapChain.h"

class Image
{
public:
	Image(Device* dev, const SwapChain* swapChain);
	~Image();

	const std::vector<VkImageView>& GetImageViews() const { return swapChainImageViews; }

private:
	void CreateImageViews(const SwapChain* swapChain);

private:
	std::vector<VkImageView> swapChainImageViews;
	Device* device;
};