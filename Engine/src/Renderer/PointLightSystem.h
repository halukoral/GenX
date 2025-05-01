#pragma once
#include "Device.h"
#include "FrameInfo.h"
#include "Pipeline.h"

class PointLightSystem
{
public:
	PointLightSystem(Device &device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout);
	~PointLightSystem();

	PointLightSystem(const PointLightSystem &) = delete;
	PointLightSystem &operator=(const PointLightSystem &) = delete;

	void Update(const FrameInfo &frameInfo, GlobalUbo &ubo);
	void Render(const FrameInfo &frameInfo) const;

private:
	void CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout);
	void CreatePipeline(VkRenderPass renderPass);

	Device &m_Device;

	std::unique_ptr<Pipeline> m_Pipeline;
	VkPipelineLayout m_PipelineLayout;
};
