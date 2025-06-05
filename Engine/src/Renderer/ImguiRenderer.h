#pragma once

#include "pch.h"
#include "Device.h"
#include "RenderPass.h"

class ImGuiRenderer
{
public:
	ImGuiRenderer(Device* dev, Window* win, RenderPass* rp, VkCommandPool* cmdPool, uint32_t minImgCount, uint32_t imgCount);
	~ImGuiRenderer();

	void NewFrame();
	void Render(VkCommandBuffer commandBuffer);
	void HandleResize() const;

private:
	void InitImGui();

	VkCommandBuffer BeginSingleTimeCommands() const;
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

	static void CheckVkResult(VkResult err);
	void Cleanup() const;

private:
	Device* m_Device;
	Window* m_Window;
	RenderPass* m_RenderPass;
	VkDescriptorPool m_DescriptorPool;
	VkCommandPool* m_CommandPool;
	uint32_t m_MinImageCount;
	uint32_t m_ImageCount;
};
