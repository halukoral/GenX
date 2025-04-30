#pragma once
#include "pch.h"
#include "Device.h"

struct PipelineConfigInfo
{
	PipelineConfigInfo() = default;

	PipelineConfigInfo(const PipelineConfigInfo&) = delete;
	PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

	std::vector<VkVertexInputBindingDescription> BindingDescriptions{};
	std::vector<VkVertexInputAttributeDescription> AttributeDescriptions{};
	
	VkPipelineViewportStateCreateInfo ViewportInfo {};
	VkPipelineInputAssemblyStateCreateInfo InputAssemblyInfo {};
	VkPipelineRasterizationStateCreateInfo RasterizationInfo {};
	VkPipelineMultisampleStateCreateInfo MultisampleInfo {};
	VkPipelineColorBlendAttachmentState ColorBlendAttachment {};
	VkPipelineColorBlendStateCreateInfo ColorBlendInfo {};
	VkPipelineDepthStencilStateCreateInfo DepthStencilInfo {};
	std::vector<VkDynamicState> DynamicStateEnables;
	VkPipelineDynamicStateCreateInfo DynamicStateInfo {};
	VkPipelineLayout PipelineLayout {};
	VkRenderPass RenderPass {};
	uint32_t Subpass = 0;
};

class Pipeline
{
public:
	Pipeline(
		Device& device,
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo);

	~Pipeline();
	
	Pipeline(const Pipeline&) = delete;
	Pipeline& operator=(const Pipeline&) = delete;

	static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
	static void EnableAlphaBlending(PipelineConfigInfo& configInfo);

	void Bind(VkCommandBuffer commandBuffer);
	
private:
	static std::vector<char> ReadFile(const std::string& filepath);
	void CreateGraphicsPipeline(
		const std::string& vertFilepath,
		const std::string& fragFilepath,
		const PipelineConfigInfo& configInfo);

	void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);

private:
	Device& m_Device;

	VkPipeline m_GraphicsPipeline;
	VkShaderModule m_VertShaderModule;
	VkShaderModule m_FragShaderModule;
};