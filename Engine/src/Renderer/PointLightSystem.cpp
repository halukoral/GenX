#include "PointLightSystem.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <ranges>

#include "ECS/Components/TransformComponent.h"

struct PointLightPushConstants
{
	glm::vec4 Position{};
	glm::vec4 Color{};
	float Radius;
};

PointLightSystem::PointLightSystem(Device& device, VkRenderPass renderPass, VkDescriptorSetLayout globalSetLayout)
	: m_Device{device}
{
	CreatePipelineLayout(globalSetLayout);
	CreatePipeline(renderPass);
}

PointLightSystem::~PointLightSystem()
{
	vkDestroyPipelineLayout(m_Device.GetLogicalDevice(), m_PipelineLayout, nullptr);
}

void PointLightSystem::CreatePipelineLayout(VkDescriptorSetLayout globalSetLayout)
{
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(PointLightPushConstants);

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

void PointLightSystem::CreatePipeline(const VkRenderPass renderPass)
{
	assert(m_PipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

	PipelineConfigInfo pipelineConfig{};
	Pipeline::DefaultPipelineConfigInfo(pipelineConfig);
	Pipeline::EnableAlphaBlending(pipelineConfig);
	pipelineConfig.AttributeDescriptions.clear();
	pipelineConfig.BindingDescriptions.clear();
	pipelineConfig.RenderPass = renderPass;
	pipelineConfig.PipelineLayout = m_PipelineLayout;
	m_Pipeline = std::make_unique<Pipeline>(
		m_Device,
		"../point_light.vert.spv",
		"../point_light.frag.spv",
		pipelineConfig);
}

void PointLightSystem::Update(const FrameInfo& frameInfo, GlobalUbo& ubo)
{
	auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * frameInfo.FrameTime, {0.f, -1.f, 0.f});
	int lightIndex = 0;
	for (auto& kv : frameInfo.GameObjects)
	{
		auto& obj = kv.second;
		if (obj.PointLight == nullptr) continue;

		//assert(lightIndex < MAX_LIGHTS && "Point lights exceed maximum specified");

		// update light position
		obj.Transform.Position = glm::vec3(rotateLight * glm::vec4(obj.Transform.Position, 1.f));

		// copy light to ubo
		ubo.PointLights[lightIndex].Position = glm::vec4(obj.Transform.Position, 1.f);
		ubo.PointLights[lightIndex].Color = glm::vec4(obj.Color, obj.PointLight->LightIntensity);

		lightIndex += 1;
	}
	ubo.NumLights = lightIndex;
}

void PointLightSystem::Render(const FrameInfo& frameInfo) const
{
	// sort lights
	std::map<float, GameObject::id_t> sorted;
	for (auto& val : frameInfo.GameObjects | std::views::values)
	{
		auto& obj = val;
		if (obj.PointLight == nullptr) continue;

		// calculate distance
		const auto transform = frameInfo.Camera.GetEntity()->GetComponent<TransformComponent>();
		auto offset = transform->Position - obj.Transform.Position;
		float disSquared = glm::dot(offset, offset);
		sorted[disSquared] = obj.GetId();
	}

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

	// iterate through sorted lights in reverse order
	for (auto& it : std::ranges::reverse_view(sorted))
	{
		// use game obj id to find light object
		auto& obj = frameInfo.GameObjects.at(it.second);

		PointLightPushConstants push{};
		push.Position = glm::vec4(obj.Transform.Position, 1.f);
		push.Color = glm::vec4(obj.Color, obj.PointLight->LightIntensity);
		push.Radius = obj.Transform.Scale.x;

		vkCmdPushConstants(
			frameInfo.CommandBuffer,
			m_PipelineLayout,
			VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			sizeof(PointLightPushConstants),
			&push);
		vkCmdDraw(frameInfo.CommandBuffer, 6, 1, 0, 0);
	}
}
