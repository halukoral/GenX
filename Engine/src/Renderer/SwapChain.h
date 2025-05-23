#pragma once

#include "pch.h"
#include "Device.h"

class SwapChain
{
    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

public:
    SwapChain(Device* dev, Window* win);
    ~SwapChain();

    VkSwapchainKHR  GetSwapChain() const    { return swapChain; }
    VkFormat        GetImageFormat() const  { return swapChainImageFormat; }
    VkExtent2D      GetExtent() const       { return swapChainExtent; }
    const std::vector<VkImage>& GetImages() const { return swapChainImages; }

private:
    void CreateSwapChain(Window* window);

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window* window);
private:
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    Device* device;
};
