#pragma once

#include "pch.h"
#include <vulkan/vulkan_core.h>

#include "Buffers.h"

class Window;

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR Capabilities;
	std::vector<VkSurfaceFormatKHR> Formats;
	std::vector<VkPresentModeKHR> PresentModes;

	[[nodiscard]] bool IsValid() const
	{
		return !Formats.empty() && !PresentModes.empty();
	}
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> GraphicsFamily = std::nullopt;
	std::optional<uint32_t> PresentationFamily = std::nullopt;

	[[nodiscard]] bool IsValid() const
	{
		return GraphicsFamily.has_value() && PresentationFamily.has_value();
	}
};

class Device
{
public:
	Device(Window &window);
	~Device();

	Device(const Device &) = delete;
	Device &operator=(const Device &) = delete;
	Device(Device &&) = delete;
	Device &operator=(Device &&) = delete;
	
	void Initialize();

	VkInstance			GetInstance() const			{ return m_Instance; }
	VkPhysicalDevice	GetPhysicalDevice() const	{ return m_PhysicalDevice; }
	VkDevice			GetLogicalDevice() const	{ return m_LogicalDevice; }
	VkQueue 			GetGraphicsQueue() const	{ return m_GraphicsQueue; }
	VkQueue 			GetPresentQueue() const		{ return m_PresentQueue; }
	VkSurfaceKHR		GetSurface() const			{ return m_Surface; }
	VkCommandPool		GetCommandPool() const		{ return m_CommandPool; }
	const Window&		GetWindow() const			{ return m_Window; }
	
	SwapChainSupportDetails GetSwapChainSupport() const { return QuerySwapChainSupport(m_PhysicalDevice); }
	QueueFamilyIndices FindPhysicalQueueFamilies() const { return FindQueueFamilies(m_PhysicalDevice); }
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
	VkFormat FindSupportedFormat(
		const std::vector<VkFormat> &candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features) const;

	// Buffer Helper Functions
	BufferHandle CreateBuffer(
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties) const;
	
	VkCommandBuffer BeginSingleTimeCommands() const;
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer) const;

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
	void CopyBufferToImage(
		VkBuffer buffer,
		VkImage image,
		uint32_t width,
		uint32_t height,
		uint32_t layerCount) const;

	void CreateImageWithInfo(
		const VkImageCreateInfo &imageInfo,
		VkMemoryPropertyFlags properties,
		VkImage &image,
		VkDeviceMemory &imageMemory) const;

	VkPhysicalDeviceProperties Properties;

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;

private:
	void CreateInstance();
	void SetupDebugMessenger();
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDevice();
	void CreateCommandPool();

	// helper functions
	bool IsDeviceSuitable(VkPhysicalDevice device);

	bool AreAllDeviceExtensionsSupported(VkPhysicalDevice device);
	bool AreAllExtensionsSupported(const gsl::span<gsl::czstring>& extensions);
	bool AreAllLayersSupported(gsl::span<gsl::czstring> layers);

	gsl::span<gsl::czstring>			GetSuggestedInstanceExtensions();
	std::vector<gsl::czstring>			GetRequiredInstanceExtensions();
	std::vector<VkExtensionProperties>	GetAvailableDeviceExtensions(VkPhysicalDevice device);
	std::vector<VkExtensionProperties>	GetSupportedInstanceExtensions();
	std::vector<VkLayerProperties>		GetSupportedValidationLayers();
	std::vector<VkPhysicalDevice>		GetAvailablePhysicalDevices() const;
	
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) const;

	Window &m_Window;

	/////////////////////////////////////////////////
	/// VULKAN
	/// To draw the first triangle we need to:
	///  1. Create VkInstance
	///  2. Select a supported graphics card (VkPhysicalDevice)
	///  3. Create a VkDevice and VkQueue for drawing and presentation
	///  4. Create a window, window surface and swap chain.
	///  5. Wrap the swap chain images into VkImageView
	///  6. Create a render pass that specifies the render targets and usage
	///  7. Create framebuffers for the render pass
	///  8. Set up the graphics pipeline
	///  9. Allocate and record a command buffer with draw commands for
	///		every possible swap chain image
	///	10. Draw frames by acquiring images, submitting the right draw command
	///		buffer and returning the images back to the swap chain.

	// 1
	VkInstance m_Instance = VK_NULL_HANDLE;

	// Now we need to look for and select a graphics card 
	// in the system that supports the features we need.
	// This object will be IMPLICITLY destroyed when VkInstance is destroyed.
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

	// Commands in Vulkan are not executed directly.
	// You have to record in command buffer objects first.
	VkCommandPool m_CommandPool;
	
	// After selecting physical device, you need to
	// set up a logical device to interface it.
	// You can even create multiple logical devices
	// from the same physical device if you have
	// varying requirements.
	VkDevice m_LogicalDevice = VK_NULL_HANDLE;

	// Since Vulkan is platform-agnostic API, it can't interface directly
	// the window system on its own. To establish the connection, we need
	// to use WSI extensions. VK_KHR_Surface extension is an instance level
	// extension, and we're actually enabled it, because it's included in the
	// list returned by glfwGetRequiredInstanceExtensions().
	// The list also includes some other WSI extensions that we will use later.
	// --
	// Window surface need to be created right after the instance creation.
	// Window surfaces are an entirely optional component in VULKAN, if you
	// just need off-screen rendering
	VkSurfaceKHR m_Surface;
	
	// Almost every operation in Vulkan to be submitted to a QUEUE.
	// Queues are IMPLICITLY cleaned up when VkInstance is destroyed.
	VkQueue m_GraphicsQueue;
	VkQueue m_PresentQueue;

	/////////////////////////////////////////////////
	/// Validation layers : (optional)
	///		* Check the values of params against the spec to detect misuse
	///		* Checking thread safety
	///		* Tracking creation and destruction of objects for mem leaks
	///		* Logging every call and its params to std output
	///		* Tracing vulkan calls for profiling and replaying
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	
	std::array<gsl::czstring, 1> m_ValidationLayers = {"VK_LAYER_KHRONOS_validation"};
	std::array<gsl::czstring, 1> m_RequiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};
