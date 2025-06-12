#pragma once

#include "pch.h"
#include "Device.h"
#include "RenderPass.h"
#include "SwapChain.h"

// Pipeline Class - Graphics pipeline yönetimi
class Pipeline
{
    struct Vertex
    {
        float pos[2];
        float color[3];
    };

public:
    Pipeline(Device* dev, SwapChain* swapChain, RenderPass* renderPass);
    ~Pipeline();

    VkPipeline GetPipeline() const { return graphicsPipeline; }
    VkPipelineLayout GetPipelineLayout() const { return pipelineLayout; }

private:
    void CreateGraphicsPipeline(SwapChain* swapChain, RenderPass* renderPass);
    VkShaderModule CreateShaderModule(const std::vector<char>& code) const;
    static std::vector<char> ReadFile(const std::string& filename);

private:
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;
    Device* device;
};
