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

    VkSwapchainKHR  GetSwapChain() const    		{ return m_SwapChain; }
    VkFormat        GetImageFormat() const  		{ return m_SwapChainImageFormat; }
    VkExtent2D      GetExtent() const       		{ return m_SwapChainExtent; }
    const std::vector<VkImage>& GetImages() const	{ return m_SwapChainImages; }

private:
    void CreateSwapChain(Window* window);

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, Window* window);
	
private:
    VkSwapchainKHR m_SwapChain;
    std::vector<VkImage> m_SwapChainImages;
    VkFormat m_SwapChainImageFormat;
    VkExtent2D m_SwapChainExtent;
    Device* m_Device;
};
