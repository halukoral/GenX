#pragma once

#include "pch.h"
#include "Device.h"
#include "FrameInfo.h"
#include "Pipeline.h"
#include "Actor/GameObject.h"

class RenderSystem
{
public:
	RenderSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~RenderSystem();

	RenderSystem(const RenderSystem &) = delete;
	RenderSystem &operator=(const RenderSystem &) = delete;

	void RenderGameObjects(FrameInfo& frameInfo);

private:
	void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void CreatePipeline(VkRenderPass renderPass);

	Device& m_Device;
	std::unique_ptr<Pipeline> m_Pipeline;
	VkPipelineLayout m_PipelineLayout;
};
