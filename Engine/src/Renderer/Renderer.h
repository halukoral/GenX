#pragma once

#include "pch.h"
#include "Device.h"
#include "SwapChain.h"

class Renderer
{
public:
	Renderer(Window& window, Device& device);
	~Renderer();

	Renderer(const Renderer &) = delete;
	Renderer &operator=(const Renderer &) = delete;

	void Initialize();

	VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
	bool IsFrameInProgress() const { return m_IsFrameStarted; }
	float GetAspectRatio() const { return m_SwapChain->ExtentAspectRatio(); }

	VkCommandBuffer GetCurrentCommandBuffer() const
	{
		//assert(isFrameStarted && "Cannot get command buffer when frame not in progress");
		return m_CommandBuffers[m_CurrentFrameIndex];
	}

	int GetFrameIndex() const
	{
		assert(m_IsFrameStarted && "Cannot get frame index when frame not in progress");
		return m_CurrentFrameIndex;
	}

	VkCommandBuffer BeginFrame();
	void EndFrame();
	void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
	void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);
	
private:
	void CreateCommandBuffers();
	void FreeCommandBuffers();
	void CreateSwapChain();

	Window &m_Window;
	Device &m_Device;
	std::unique_ptr<SwapChain> m_SwapChain;
	std::vector<VkCommandBuffer> m_CommandBuffers;

	uint32_t m_CurrentImageIndex;
	int m_CurrentFrameIndex {0};
	bool m_IsFrameStarted {false};
};
