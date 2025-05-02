#include "Device.h"

#include "Core.h"
#include "Window.h"
#include "spdlog/spdlog.h"

#pragma region VALIDATION_LAYERS

#pragma region VK_FUNCTION_EXT_IMPL

// Proxy function
VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
	VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* info,
	const VkAllocationCallbacks* allocator,
	VkDebugUtilsMessengerEXT* debugMessenger)
{
	PFN_vkCreateDebugUtilsMessengerEXT function =
		reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

	if (function != nullptr)
	{
		return function(instance, info, allocator, debugMessenger);
	}
	return VK_ERROR_EXTENSION_NOT_PRESENT;
}

// Proxy function
VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
	VkInstance instance,
	VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* allocator)
{
	PFN_vkDestroyDebugUtilsMessengerEXT function =
		reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
			vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

	if (function != nullptr)
	{
		function(instance, debugMessenger, allocator);
	}
}

#pragma endregion

namespace
{
	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data)
	{
		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			spdlog::warn("Vulkan Validation: {}", callback_data->pMessage);
		}
		else
		{
			spdlog::error("Vulkan Error: {}", callback_data->pMessage);
		}

		return VK_FALSE;
	}

	VkDebugUtilsMessengerCreateInfoEXT CreateMessengerInfo()
	{
		VkDebugUtilsMessengerCreateInfoEXT creation_info = {};
		creation_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		creation_info.pNext = nullptr;

		creation_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

		creation_info.messageType =		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
										VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

		creation_info.pfnUserCallback = DebugCallback;
		creation_info.pUserData = nullptr;

		return creation_info;
	}
}

void Device::SetupDebugMessenger()
{
	if (!EnableValidationLayers)
	{
		return;
	}

	const VkDebugUtilsMessengerCreateInfoEXT createInfo = CreateMessengerInfo();
	if (vkCreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger) != VK_SUCCESS)
	{
		spdlog::error("Cannot create debug messenger");
		return;
	}
}

#pragma endregion

Device::Device(Window &window) : m_Window{window}
{
}

Device::~Device()
{
	vkDestroyCommandPool(m_LogicalDevice, m_CommandPool, nullptr);

	// Logical devices don't interact directly with instances,
	// which is why it's not included as a parameter.
	vkDestroyDevice(m_LogicalDevice, nullptr);

	if (EnableValidationLayers)
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);

	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

	// ------------------------------------------------------------------------- \\
	// ALL OTHER VULKAN RESOURCES should be cleaned up before Instance deletion. \\
	// ------------------------------------------------------------------------- \\
	
	// VkInstance should be the LAST destroyed.
	vkDestroyInstance(m_Instance, nullptr);
}

void Device::Initialize()
{
	// Order matters
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDevice();
	CreateCommandPool();
}

void Device::CreateInstance()
{
	if (!AreAllLayersSupported(m_ValidationLayers))
	{
		EnableValidationLayers = false;
	}

	// A lot of information in Vulkan is passed through structs instead of func params.
	
	// optional
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "GenX Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	// required
	VkInstanceCreateInfo createInfo{};
	// we need to explicitly specify the type 
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	// Since Vulkan is a platform-agnostic API, you need 
	// an extension to interface with the window system
	// in our project, we need swap chain (m_RequiredDeviceExtensions)
	const std::vector<gsl::czstring> requiredExtensions = GetRequiredInstanceExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	const VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = CreateMessengerInfo();
	if (EnableValidationLayers)
	{
		createInfo.pNext = &debugCreateInfo;
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
		createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	// INSTANCE CREATION
	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}

	// As you see, the general pattern that object creation func params in Vulkan:
	//	* ptr to struct with creation info
	//  * ptr to custom allocator callbacks
	//  * ptr to variable that stores the handle to the new object.
}

std::vector<VkPhysicalDevice> Device::GetAvailablePhysicalDevices() const
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

	if (deviceCount == 0)
	{
		return {};
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

	return devices;
}

void Device::PickPhysicalDevice()
{
	// Explanation of C++ code:
	// -----------------------------------------------
	// bool is_same(int a, int b) noexcept { return a == b; }
	//
	// auto is_differ = std::not_fn(is_same);
	// assert(is_differ(8, 8) == false); // equivalent to: !is_same(8, 8) == false
	// -----------------------------------------------
	// int minus(int a, int b) { return a - b; }
	//
	// auto fifty_minus = std::bind_front(minus, 50);
	// assert(fifty_minus(3) == 47); // equivalent to: minus(50, 3) == 47
	// -----------------------------------------------

	auto devices = GetAvailablePhysicalDevices();

	// Erases all elements that satisfy the predicate pred from the container.
	// Equivalent to:
	// auto it = std::remove_if(c.begin(), c.end(), pred);
	// auto r = c.end() - it;
	// c.erase(it, c.end());
	// return r;
	std::erase_if(devices, std::not_fn( std::bind_front(&Device::IsDeviceSuitable, this) ));

	if (devices.empty())
	{
		throw std::runtime_error("failed to get physical device list!");
	}

	m_PhysicalDevice = devices[0];

	VkPhysicalDeviceProperties properties{};
	vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
	std::cout << "physical device: " << properties.deviceName << '\n';
}

void Device::CreateLogicalDevice()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	if (!indices.IsValid())
	{
		spdlog::error("failed to get queue families!");
		std::exit(EXIT_FAILURE);
	}

	std::set uniqueQueueFamilies =
	{
		indices.GraphicsFamily.value(),
		indices.PresentationFamily.value()
	};

	// Vulkan lets you assign priorities to queues to
	// influence the scheduling of command buffer execution.
	// This is required even if there is only a single queue:
	float queuePriority = 1.0f;
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		// VkDeviceQueueCreateInfo describes the number of queues we want for a single queue family.
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	// We also need to specify the set of device features that we’ll be using
	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	
	// With the previous two structures in place, we can
	// start filling in the VkDeviceCreateInfo structure.
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = static_cast<uint32_t>(m_RequiredDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = m_RequiredDeviceExtensions.data();

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	// Retrieving queue handles.
	vkGetDeviceQueue(m_LogicalDevice, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_LogicalDevice, indices.PresentationFamily.value(), 0, &m_PresentQueue);
}

void Device::CreateCommandPool()
{
	const QueueFamilyIndices queueFamilyIndices = FindPhysicalQueueFamilies();

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily.value();
	poolInfo.flags =
	  VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

	if (vkCreateCommandPool(m_LogicalDevice, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create command pool!");
	}
}

void Device::CreateSurface()
{
	m_Window.CreateWindowSurface(m_Instance, &m_Surface);
}

#pragma region DEVICE_SUITABLE

namespace 
{
	bool IsExtensionNameEqual(const gsl::czstring name, const VkExtensionProperties& extension)
	{
		return Utils::streq(extension.extensionName, name);
	}

	bool IsExtensionSupported(gsl::span<VkExtensionProperties> extensions, gsl::czstring name)
	{
		return std::ranges::any_of(extensions,std::bind_front(IsExtensionNameEqual, name));
	}

	bool IsLayerNameEqual(gsl::czstring name, const VkLayerProperties& properties)
	{
		return Utils::streq(properties.layerName, name);
	}

	bool IsLayerSupported(gsl::span<VkLayerProperties> layers, gsl::czstring name)
	{
		return std::ranges::any_of(layers, std::bind_front(IsLayerNameEqual, name));
	}
}

bool Device::IsDeviceSuitable(VkPhysicalDevice device)
{
	const QueueFamilyIndices indices = FindQueueFamilies(device);
	
	return indices.IsValid() && AreAllDeviceExtensionsSupported(device);
}

bool Device::AreAllLayersSupported(gsl::span<gsl::czstring> layers)
{
	auto supportedLayers = GetSupportedValidationLayers();

	return std::ranges::all_of(layers, std::bind_front(IsLayerSupported, supportedLayers));
}

bool Device::AreAllDeviceExtensionsSupported(const VkPhysicalDevice device)
{
	auto availableExtensions = GetAvailableDeviceExtensions(device);

	return std::ranges::all_of(m_RequiredDeviceExtensions,std::bind_front(IsExtensionSupported, availableExtensions));
}

gsl::span<gsl::czstring> Device::GetSuggestedInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	gsl::czstring* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	return {glfwExtensions, glfwExtensionCount};
}

std::vector<gsl::czstring> Device::GetRequiredInstanceExtensions()
{
	gsl::span<gsl::czstring> suggestedExtensions = GetSuggestedInstanceExtensions();
	std::vector<gsl::czstring> requiredExtensions(suggestedExtensions.size());
	std::ranges::copy(suggestedExtensions, requiredExtensions.begin());

	if (EnableValidationLayers)
	{
		requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	if (!AreAllExtensionsSupported(requiredExtensions))
	{
		throw std::runtime_error("extensions not supported!");
	}

	return requiredExtensions;
}

std::vector<VkExtensionProperties> Device::GetAvailableDeviceExtensions(const VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
	return extensions;
}

bool Device::AreAllExtensionsSupported(const gsl::span<gsl::czstring>& extensions)
{
	auto supportedExtensions = GetSupportedInstanceExtensions();

	return std::ranges::all_of(extensions,std::bind_front(IsExtensionSupported, supportedExtensions));
}

std::vector<VkExtensionProperties> Device::GetSupportedInstanceExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	if (extensionCount == 0) return {};

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	return availableExtensions;
}

std::vector<VkLayerProperties> Device::GetSupportedValidationLayers()
{
	uint32_t count;
	vkEnumerateInstanceLayerProperties(&count, nullptr);

	if (count == 0) return {};

	std::vector<VkLayerProperties> properties(count);
	vkEnumerateInstanceLayerProperties(&count, properties.data());
	return properties;
}
#pragma endregion

QueueFamilyIndices Device::FindQueueFamilies(const VkPhysicalDevice device) const
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	const auto iter =
		std::ranges::find_if(queueFamilies, [](const VkQueueFamilyProperties& props)
		{
		  return props.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
		});

	QueueFamilyIndices result;
	result.GraphicsFamily = iter - queueFamilies.begin();

	for (uint32_t i = 0; i < queueFamilies.size(); i++)
	{
		VkBool32 hasPresentationSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &hasPresentationSupport);
		if (hasPresentationSupport)
		{
			result.PresentationFamily = i;
			break;
		}
	}

	return result;
}

SwapChainSupportDetails Device::QuerySwapChainSupport(const VkPhysicalDevice device) const
{
	SwapChainSupportDetails details;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

	if (formatCount != 0)
	{
		details.Formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		details.PresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			device,
			m_Surface,
			&presentModeCount,
			details.PresentModes.data());
	}
	return details;
}

VkFormat Device::FindSupportedFormat(
	const std::vector<VkFormat> &candidates,
	const VkImageTiling tiling,
	const VkFormatFeatureFlags features) const
{
	for (const VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}
	throw std::runtime_error("failed to find supported format!");
}

uint32_t Device::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
{
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memoryProperties);
	const gsl::span<VkMemoryType> memoryTypes(memoryProperties.memoryTypes, memoryProperties.memoryTypeCount);

	for (uint32_t i = 0; i < memoryTypes.size(); i++)
	{
		const bool passesFilter = typeFilter & (1 << i);
		const bool hasPropertyFlags = memoryTypes[i].propertyFlags & properties;

		if (passesFilter && hasPropertyFlags)
			return i;
	}

	throw std::runtime_error("Cannot find memory type!");
}

BufferHandle Device::CreateBuffer(
	const VkDeviceSize size,
	const VkBufferUsageFlags usage,
	const VkMemoryPropertyFlags properties) const
{
	BufferHandle handle = {};

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	// CREATE BUFFER
	if (vkCreateBuffer(m_LogicalDevice, &bufferCreateInfo, nullptr, &handle.Buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to create vertex buffer!");
	}

	// GET MEMORY REQUIREMENTS
	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements(m_LogicalDevice, handle.Buffer, &memoryRequirements);

	// FIND MEMORY TYPE
	const uint32_t chosenMemoryType = FindMemoryType(memoryRequirements.memoryTypeBits, properties);

	VkMemoryAllocateInfo allocation_info = {};
	allocation_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocation_info.allocationSize = memoryRequirements.size;
	allocation_info.memoryTypeIndex = chosenMemoryType;

	// ALLOCATE MEMORY
	if (vkAllocateMemory(m_LogicalDevice, &allocation_info, nullptr, &handle.Memory) != VK_SUCCESS)
	{
		throw std::runtime_error("Failed to allocate buffer memory!");
	}

	// BIND MEMORY
	vkBindBufferMemory(m_LogicalDevice, handle.Buffer, handle.Memory, 0);
	return handle;
}

VkCommandBuffer Device::BeginSingleTimeCommands() const
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_LogicalDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void Device::EndSingleTimeCommands(const VkCommandBuffer commandBuffer) const
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_GraphicsQueue);

	vkFreeCommandBuffers(m_LogicalDevice, m_CommandPool, 1, &commandBuffer);
}

void Device::CopyBuffer(const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size) const
{
	const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;  // Optional
	copyRegion.dstOffset = 0;  // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndSingleTimeCommands(commandBuffer);
}

void Device::CopyBufferToImage(
	const VkBuffer buffer,
	const VkImage image,
	const uint32_t width,
	const uint32_t height,
	const uint32_t layerCount) const
{
	const VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = {width, height, 1};

	vkCmdCopyBufferToImage(
		commandBuffer,
		buffer,
		image,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		1,
		&region);
	EndSingleTimeCommands(commandBuffer);
}


void Device::CreateImageWithInfo(
	const VkImageCreateInfo &imageInfo,
	const VkMemoryPropertyFlags properties,
	VkImage &image,
	VkDeviceMemory &imageMemory) const
{
	if (vkCreateImage(m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(m_LogicalDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate image memory!");
	}

	if (vkBindImageMemory(m_LogicalDevice, image, imageMemory, 0) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to bind image memory!");
	}
}
