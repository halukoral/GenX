#pragma once

#include "Device.h"
#include <vulkan/vulkan.h>

#include "Image.h"

class SwapChain
{
public:
	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

	SwapChain(const std::shared_ptr<Device>& device, VkExtent2D windowExtent);
	SwapChain(const std::shared_ptr<Device>& device, VkExtent2D windowExtent, std::shared_ptr<SwapChain> previous);
	~SwapChain();

	SwapChain(const SwapChain &) = delete;
	SwapChain& operator=(const SwapChain &) = delete;

	void Initialize();
	
	VkFramebuffer	GetFrameBuffer(int index) const { return m_SwapChainFramebuffers[index]; }
	VkRenderPass	GetRenderPass() const			{ return m_RenderPass; }
	VkImageView		GetImageView(int index) const	{ return m_SwapChainImages[index].GetImageView(); }
	VkFormat		GetSwapChainImageFormat() const { return m_SwapChainImageFormat; }
	VkExtent2D		GetSwapChainExtent() const		{ return m_SwapChainExtent; }
	size_t			ImageCount() const				{ return m_SwapChainImages.size(); }
	uint8_t			GetCurrentFrame() const			{ return m_CurrentFrame; }
	uint32_t		Width() const					{ return m_SwapChainExtent.width; }
	uint32_t		Height() const					{ return m_SwapChainExtent.height; }

	float ExtentAspectRatio() const
	{
		return static_cast<float>(m_SwapChainExtent.width) / static_cast<float>(m_SwapChainExtent.height);
	}
	VkFormat FindDepthFormat();

	VkResult AcquireNextImage(uint32_t *imageIndex);
	VkResult SubmitCommandBuffers(const VkCommandBuffer *buffers, uint32_t *imageIndex);

	bool CompareSwapFormats(const SwapChain& swapChain) const
	{
		return swapChain.m_SwapChainDepthFormat == m_SwapChainDepthFormat &&
			   swapChain.m_SwapChainImageFormat == m_SwapChainImageFormat;
	}
	
private:
	void CreateSwapChain();
	void CreateImageViews();
	void CreateDepthResources();
	void CreateRenderPass();
	void CreateFramebuffers();
	void CreateSyncObjects();

	// Helper functions
	VkSurfaceFormatKHR	ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
	VkPresentModeKHR	ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
	VkExtent2D			ChooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

	VkFormat m_SwapChainImageFormat;
	VkFormat m_SwapChainDepthFormat;
	
	VkExtent2D m_SwapChainExtent;

	std::vector<VkFramebuffer> m_SwapChainFramebuffers;
	VkRenderPass m_RenderPass;

	std::vector<Image> m_DepthImages;
	std::vector<Image> m_SwapChainImages;

	std::shared_ptr<Device> m_Device;
	VkExtent2D m_WindowExtent;

	VkSwapchainKHR m_SwapChain;
	std::shared_ptr<SwapChain> m_OldSwapChain;
	
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;
	std::vector<VkFence> m_ImagesInFlight;
	uint8_t m_CurrentFrame = 0;
};
