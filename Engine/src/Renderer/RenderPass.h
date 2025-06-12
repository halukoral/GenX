#pragma once

#include "pch.h"
#include "Device.h"
#include "SwapChain.h"

class RenderPass
{
public:
    RenderPass(Device* dev, SwapChain* swapChain);
    ~RenderPass();

    VkRenderPass GetRenderPass() const;

private:
    void CreateRenderPass(SwapChain* swapChain);

private:
    VkRenderPass renderPass;
    Device* device;
};