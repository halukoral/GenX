#pragma once

#include "pch.h"
#include "Window.h"

class Device
{
	#ifdef NDEBUG
		const bool enableValidationLayers = false;
	#else
		const bool enableValidationLayers = true;
	#endif

	struct QueueFamilyIndices
	{
		uint32_t graphicsFamily = UINT32_MAX;
		uint32_t presentFamily = UINT32_MAX;

		bool IsComplete() const
		{
			return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
		}
	};

public:
	Device(Window* win);
	~Device();

	VkDevice			GetDevice() const { return device; }
	VkPhysicalDevice	GetPhysicalDevice() const { return physicalDevice; }
	VkQueue				GetGraphicsQueue() const { return graphicsQueue; }
	VkQueue				GetPresentQueue() const { return presentQueue; }
	VkSurfaceKHR		GetSurface() const { return surface; }

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

private:
	void CreateInstance();
	void CreateSurface(Window* window);
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
	void CreateLogicalDevice();

private:
	VkInstance instance;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device;
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkSurfaceKHR surface;

	const std::vector<const char*> validationLayers = {	"VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> deviceExtensions = {	VK_KHR_SWAPCHAIN_EXTENSION_NAME	};
};
