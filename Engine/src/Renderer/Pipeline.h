#pragma once

#include "pch.h"
#include "Device.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "Model.h"

// Pipeline Class - Graphics pipeline yönetimi
class Pipeline
{
public:
	Pipeline(Device* dev, SwapChain* swapChain, RenderPass* renderPass, VkDescriptorSetLayout descriptorSetLayout);
	~Pipeline();

	VkPipeline GetPipeline() const { return graphicsPipeline; }
	VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

private:
	void CreateGraphicsPipeline(SwapChain* swapChain, RenderPass* renderPass, VkDescriptorSetLayout descriptorSetLayout);
	VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
	static std::vector<char> ReadFile(const std::string& filename);

private:
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;
	Device* device;
};