#pragma once
#include "RenderSystem.h"

enum class RenderPassState
{
	READY,
	RECORDING,
	IN_RENDER_PASS,
	RECORDING_ENDED,
	SUBMITTED,
	NOT_ALLOCATED
};

class RenderPass
{
public:
	RenderPass(const std::shared_ptr<Device>& device);
	void Destroy() const;
	
	void CreateRenderPass(VkFormat swapChainFormat, VkFormat depthFormat);

	VkRenderPass GetRenderPass() const { return m_RenderPass; }

	void Begin(VkFramebuffer framebuffer, VkCommandBuffer commandBuffer, VkExtent2D extent);
	void End();
	
private:
	std::shared_ptr<Device> m_Device;
	
	VkRenderPass m_RenderPass {VK_NULL_HANDLE};
};
