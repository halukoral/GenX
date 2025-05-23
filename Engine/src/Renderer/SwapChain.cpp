#include "SwapChain.h"

SwapChain::SwapChain(const std::shared_ptr<Device>& device, const VkExtent2D windowExtent)
	: m_RenderPass{device}, m_Device{device}, m_WindowExtent{windowExtent}
{
	Initialize();
}

SwapChain::SwapChain(const std::shared_ptr<Device>& device, const VkExtent2D windowExtent, std::shared_ptr<SwapChain> previous)
	: m_RenderPass{device}, m_Device{device}, m_WindowExtent{windowExtent}
{
	Initialize();
	m_OldSwapChain = nullptr;
}

SwapChain::~SwapChain()
{
	if (m_SwapChain != nullptr)
	{
		vkDestroySwapchainKHR(m_Device->GetLogicalDevice(), m_SwapChain, nullptr);
		m_SwapChain = nullptr;
	}

	for (auto& image : m_SwapChainImages)
	{
		image.Shutdown();
	}

	for (auto& image : m_DepthImages)
	{
		image.Shutdown();
	}

	for (const auto framebuffer : m_SwapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_Device->GetLogicalDevice(), framebuffer, nullptr);
	}

	m_RenderPass.Destroy();
	
	// cleanup synchronization objects
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_Device->GetLogicalDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device->GetLogicalDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_Device->GetLogicalDevice(), m_InFlightFences[i], nullptr);
	}
}

void SwapChain::Initialize()
{
	CreateSwapChain();
	CreateImageViews();
	CreateRenderPass();
	CreateDepthResources();
	CreateFramebuffers();
	CreateSyncObjects();
}

VkResult SwapChain::AcquireNextImage(uint32_t *imageIndex) const
{
	vkWaitForFences(
		m_Device->GetLogicalDevice(),
		1,
		&m_InFlightFences[m_CurrentFrame],
		VK_TRUE,
		std::numeric_limits<uint64_t>::max());

	const VkResult result = vkAcquireNextImageKHR(
		m_Device->GetLogicalDevice(),
		m_SwapChain,
		std::numeric_limits<uint64_t>::max(),
		m_ImageAvailableSemaphores[m_CurrentFrame],  // must be a not signaled semaphore
		VK_NULL_HANDLE,
		imageIndex);

	return result;
}

VkResult SwapChain::SubmitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex)
{
	if (m_ImagesInFlight[*imageIndex] != VK_NULL_HANDLE) {
		vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_ImagesInFlight[*imageIndex], VK_TRUE, UINT64_MAX);
	}
	m_ImagesInFlight[*imageIndex] = m_InFlightFences[m_CurrentFrame];

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = buffers;

	VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_CurrentFrame]);
	if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {m_SwapChain};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;

	presentInfo.pImageIndices = imageIndex;

	const auto result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	return result;
}

bool SwapChain::CompareSwapFormats(const SwapChain& swapChain) const
{
	return	swapChain.m_SwapChainDepthFormat == m_SwapChainDepthFormat &&
			swapChain.m_SwapChainImageFormat == m_SwapChainImageFormat;
}

void SwapChain::CreateSwapChain()
{
	const SwapChainSupportDetails swapChainSupport = m_Device->GetSwapChainSupport();

	const VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.Formats);
	const VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.PresentModes);
	const VkExtent2D extent = ChooseSwapExtent(swapChainSupport.Capabilities);

	uint32_t imageCount = swapChainSupport.Capabilities.minImageCount + 1;
	if (swapChainSupport.Capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.Capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.Capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = m_Device->GetSurface();

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	// Setup the queue family indices
	const QueueFamilyIndices indices = m_Device->FindPhysicalQueueFamilies();
	const uint32_t queueFamilyIndices[] = {indices.GraphicsFamily.value(), indices.PresentationFamily.value()};

	if (indices.GraphicsFamily != indices.PresentationFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0;      // Optional
		createInfo.pQueueFamilyIndices = nullptr;  // Optional
	}

	createInfo.preTransform = swapChainSupport.Capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = m_OldSwapChain == nullptr ?
		VK_NULL_HANDLE :
		m_OldSwapChain->m_SwapChain;

	if (vkCreateSwapchainKHR(m_Device->GetLogicalDevice(), &createInfo, nullptr, &m_SwapChain) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create swap chain!");
	}

	// Images
	// ------
	// We only specified a minimum number of images in the swap chain, so the implementation is
	// allowed to create a swap chain with more. That's why we'll first query the final number of
	// images with vkGetSwapchainImagesKHR, then resize the container and finally call it again to
	// retrieve the handles.
	vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_SwapChain, &imageCount, nullptr);
	m_SwapChainImages.resize(imageCount);
	std::vector<VkImage> images(imageCount);
	vkGetSwapchainImagesKHR(m_Device->GetLogicalDevice(), m_SwapChain, &imageCount, images.data());

	for (size_t i = 0; i < imageCount; i++)
	{
		m_SwapChainImages[i] = Image(m_Device, images[i]);
	}

	m_SwapChainImageFormat = surfaceFormat.format;
	m_SwapChainExtent = extent;
}

void SwapChain::CreateImageViews()
{
	for (auto& swapChainImage : m_SwapChainImages)
	{
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = swapChainImage.GetImage();
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = m_SwapChainImageFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(
			m_Device->GetLogicalDevice(),
			&viewInfo,
			nullptr,
			&swapChainImage.GetImageViewRef()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture image view!");
		}
	}
}

void SwapChain::CreateRenderPass()
{
	m_RenderPass.CreateRenderPass(GetSwapChainImageFormat(), FindDepthFormat());
}

void SwapChain::CreateFramebuffers()
{
	m_SwapChainFramebuffers.resize(GetImageCount());
	for (size_t i = 0; i < GetImageCount(); i++)
	{
		std::array<VkImageView, 2> attachments = {m_SwapChainImages[i].GetImageView(), m_DepthImages[i].GetImageView()};

		VkExtent2D swapChainExtent = GetSwapChainExtent();
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass.GetRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(
				m_Device->GetLogicalDevice(),
				&framebufferInfo,
				nullptr,
				&m_SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

void SwapChain::CreateDepthResources()
{
	const VkFormat depthFormat = FindDepthFormat();
	m_SwapChainDepthFormat = depthFormat;
	const VkExtent2D swapChainExtent = GetSwapChainExtent();

	m_DepthImages.resize(m_SwapChainImages.size());
	for (auto& depthImage : m_DepthImages)
	{
		depthImage.SetDevice(m_Device);
		
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = swapChainExtent.width;
		imageInfo.extent.height = swapChainExtent.height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = depthFormat;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.flags = 0;

		depthImage.CreateImage(
			imageInfo,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			depthImage.GetImageRef(),
			depthImage.GetImageMemoryRef());

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = depthImage.GetImage();
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = depthFormat;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(m_Device->GetLogicalDevice(), &viewInfo, nullptr, &depthImage.GetImageViewRef()) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create texture image view!");
		}
	}
}

void SwapChain::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	m_ImagesInFlight.resize(GetImageCount(), VK_NULL_HANDLE);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		if (vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) !=
				VK_SUCCESS ||
			vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) !=
				VK_SUCCESS ||
			vkCreateFence(m_Device->GetLogicalDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}
}

VkSurfaceFormatKHR SwapChain::ChooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR> &availableFormats)
{
	for (const auto &availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR SwapChain::ChooseSwapPresentMode(
	const std::vector<VkPresentModeKHR> &availablePresentModes)
{
	for (const auto &availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			std::cout << "Present mode: Mailbox" << '\n';
			return availablePresentMode;
		}
	}

	std::cout << "Present mode: V-Sync" << '\n';
	return VK_PRESENT_MODE_FIFO_KHR;
}


VkExtent2D SwapChain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) const
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}

	VkExtent2D actualExtent = m_WindowExtent;
	actualExtent.width = std::max(
		capabilities.minImageExtent.width,
		std::min(capabilities.maxImageExtent.width, actualExtent.width));
	actualExtent.height = std::max(
		capabilities.minImageExtent.height,
		std::min(capabilities.maxImageExtent.height, actualExtent.height));

	return actualExtent;
}

float SwapChain::GetExtentAspectRatio() const
{
	return static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height);
}

VkFormat SwapChain::FindDepthFormat() const
{
	return m_Device->FindSupportedFormat(
		{VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}
