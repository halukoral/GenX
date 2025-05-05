#include "RenderSystem.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <ranges>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

struct SimplePushConstantData
{
	glm::mat4 modelMatrix{1.f};
	glm::mat4 normalMatrix{1.f};
};

RenderSystem::RenderSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	: m_Device{device}
{
	CreatePipelineLayout(globalSetLayout);
	CreatePipeline(renderPass);
}

RenderSystem::~RenderSystem()
{
	vkDestroyPipelineLayout(m_Device.GetLogicalDevice(), m_PipelineLayout, nullptr);
}

void RenderSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(SimplePushConstantData);

	std::vector<VkDescriptorSetLayout> descriptorSetLayouts{globalSetLayout};

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
	if (vkCreatePipelineLayout(m_Device.GetLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout) !=
		VK_SUCCESS)
	{
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void RenderSystem::CreatePipeline(VkRenderPass renderPass)
{
	assert(m_PipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
	pipelineConfig.RenderPass = renderPass;
	pipelineConfig.PipelineLayout = m_PipelineLayout;
	pipelineConfig.AttributeDescriptions.clear();
	pipelineConfig.AttributeDescriptions = Model::Vertex::GetAttributeDescriptions();
	pipelineConfig.BindingDescriptions.clear();
	pipelineConfig.BindingDescriptions = Model::Vertex::GetBindingDescriptions();
	m_Pipeline = std::make_unique<Pipeline>(
		m_Device,
		"../basic.vert.spv",
		"../basic.frag.spv",
		pipelineConfig);
}

void RenderSystem::RenderGameObjects(const FrameInfo &frameInfo) const
{
	m_Pipeline->Bind(frameInfo.CommandBuffer);

	vkCmdBindDescriptorSets(
		frameInfo.CommandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		m_PipelineLayout,
		0,
		1,
		&frameInfo.GlobalDescriptorSet,
		0,
		nullptr);

	for (auto& val : frameInfo.GameObjects | std::views::values)
	{
		auto& obj = val;
		if (obj.Model == nullptr) continue;

		const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(obj.Transform.GetTransform())));
		SimplePushConstantData push{};
		push.modelMatrix = obj.Transform.GetTransform();
		push.normalMatrix = normalMatrix;

		vkCmdPushConstants(
			frameInfo.CommandBuffer,
			m_PipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(SimplePushConstantData),
			&push);
		obj.Model->Bind(frameInfo.CommandBuffer);
		obj.Model->Draw(frameInfo.CommandBuffer);
	}
}
