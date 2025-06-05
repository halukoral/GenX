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
		uint32_t GraphicsFamily = UINT32_MAX;
		uint32_t PresentFamily = UINT32_MAX;

		bool IsComplete() const
		{
			return GraphicsFamily != UINT32_MAX && PresentFamily != UINT32_MAX;
		}
	};

public:
	Device(Window* win);
	~Device();

	VkInstance			GetInstance() const { return m_Instance; }
	VkDevice			GetLogicalDevice() const { return m_Device; }
	VkPhysicalDevice	GetPhysicalDevice() const { return m_PhysicalDevice; }
	VkQueue				GetGraphicsQueue() const { return m_GraphicsQueue; }
	VkQueue				GetPresentQueue() const { return m_PresentQueue; }
	VkSurfaceKHR		GetSurface() const { return m_Surface; }
	VkCommandPool		GetCommandPool() const { return m_CommandPool; }
	
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

private:
	void CreateInstance();
	void CreateSurface(const Window* window);
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device) const;
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
	void CreateLogicalDevice();

private:
	VkInstance m_Instance = VK_NULL_HANDLE;
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice m_Device = VK_NULL_HANDLE;
	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	VkCommandPool m_CommandPool = VK_NULL_HANDLE;
	
	const std::vector<const char*> m_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };
	const std::vector<const char*> m_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};
