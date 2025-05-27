#pragma once

#include "pch.h"
#include "Descriptor.h"
#include "Device.h"
#include "RenderPass.h"
#include "SwapChain.h"

// Pipeline Class - Graphics pipeline yönetimi
class Pipeline
{
public:
    Pipeline(Device* dev, SwapChain* swapChain, RenderPass* renderPass, Descriptor* descriptor);
    ~Pipeline();

    VkPipeline GetPipeline() const { return graphicsPipeline; }
    VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

private:
    void CreateGraphicsPipeline(SwapChain* swapChain, RenderPass* renderPass, Descriptor* descriptor);
    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
    static std::vector<char> ReadFile(const std::string& filename);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    Device* device;
};
