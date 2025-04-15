#include "Application.h"
#include "Core.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>

extern bool g_ApplicationRunning;
static Application* s_Instance = nullptr;

#pragma region VALIDATION_LAYERS

#pragma region VK_FUNCTION_EXT_IMPL

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

static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationCallback(
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

static VkDebugUtilsMessengerCreateInfoEXT GetCreateMessengerInfo()
{
	VkDebugUtilsMessengerCreateInfoEXT creation_info = {};
	creation_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	creation_info.pNext = nullptr;

	creation_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	creation_info.messageType =		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
									VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;

	creation_info.pfnUserCallback = ValidationCallback;
	creation_info.pUserData = nullptr;

	return creation_info;
}

namespace
{
	void GlfwErrorCallback(int error, const char* description)
	{
		spdlog::error ("Glfw Validation: {}", description);
	}

	bool IsLayerNameEqual(gsl::czstring name, const VkLayerProperties& properties)
	{
		return streq(properties.layerName, name);
	}

	bool IsLayerSupported(gsl::span<VkLayerProperties> layers, gsl::czstring name)
	{
		return std::ranges::any_of(layers, std::bind_front(IsLayerNameEqual, name));
	}
}

bool Application::AreAllLayersSupported(gsl::span<gsl::czstring> layers)
{
	auto supportedLayers = GetSupportedValidationLayers();

	return std::ranges::all_of(layers, std::bind_front(IsLayerSupported, supportedLayers));
}

void Application::SetupDebugMessenger()
{
	if (!EnableValidationLayers)
	{
		return;
	}

	VkDebugUtilsMessengerCreateInfoEXT info = GetCreateMessengerInfo();
	VkResult result = vkCreateDebugUtilsMessengerEXT(m_Instance, &info, nullptr, &m_DebugMessenger);
	if (result != VK_SUCCESS)
	{
		spdlog::error("Cannot create debug messenger");
		return;
	}
}

#pragma endregion

Application::Application(AppSpec spec) : m_Spec(std::move(spec))
{
	s_Instance = this;
	Init();
}

Application::~Application()
{
	Shutdown();
	s_Instance = nullptr;
}

Application& Application::Get()
{
	return *s_Instance;
}

void Application::Init()
{
	// Setup GLFW window
	glfwSetErrorCallback(GlfwErrorCallback);
	if (!glfwInit())
	{
		spdlog::error("Failed to initialize GLFW!");
		return;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_Window = glfwCreateWindow((int)m_Spec.Width, (int)m_Spec.Height, m_Spec.Name.c_str(), nullptr, nullptr);

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		spdlog::error("GLFW: Vulkan not supported!");
		return;
	}

	if (!InitVulkan())
	{
		spdlog::error("Failed to initialize Vulkan!");
		return;
	}
}

bool Application::InitVulkan()
{
	CreateInstance();
	SetupDebugMessenger();
	CreateSurface();
	PickPhysicalDevice();
	CreateLogicalDeviceAndQueues();
	CreateSwapChain();
	
	return true;
}

void Application::Shutdown()
{
	for (const auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();

	if (EnableValidationLayers)
	{
		vkDestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
	}

	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	vkDestroyDevice(m_LogicalDevice, nullptr);
	vkDestroyInstance(m_Instance, nullptr);

	glfwDestroyWindow(m_Window);
	glfwTerminate();

	g_ApplicationRunning = false;
}

void Application::Run()
{
	m_Running = true;

	// Main loop
	while (!glfwWindowShouldClose(m_Window) && m_Running)
	{
		glfwPollEvents();
		
		for (const auto& layer : m_LayerStack)
			layer->OnUpdate(m_TimeStep);

		////////////////////////////////////
		// Render

		////////////////////////////////////

		const float time = GetTime();
		m_FrameTime = time - m_LastFrameTime;
		m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
		m_LastFrameTime = time;
	}
}

void Application::Close()
{
	m_Running = false;
}

float Application::GetTime()
{
	return (float)glfwGetTime();
}

#pragma region INSTANCE_AND_EXTENSIONS

void Application::CreateInstance()
{
	std::array<gsl::czstring, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	if (!AreAllLayersSupported(validationLayers))
	{
		EnableValidationLayers = false;
	}
	std::vector<gsl::czstring> requiredExtensions = GetRequiredInstanceExtensions();

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Vulkan Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "GenX Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
	createInfo.ppEnabledExtensionNames = requiredExtensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = GetCreateMessengerInfo();
	if (EnableValidationLayers)
	{
		createInfo.pNext = &debugCreateInfo;
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}

	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

gsl::span<gsl::czstring> Application::GetSuggestedInstanceExtensions()
{
	uint32_t glfwExtensionCount = 0;
	gsl::czstring* glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	return {glfwExtensions, glfwExtensionCount};
}

std::vector<gsl::czstring> Application::GetRequiredInstanceExtensions()
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
		std::exit(EXIT_FAILURE);
	}

	return requiredExtensions;
}

std::vector<VkExtensionProperties> Application::GetSupportedInstanceExtensions()
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

	if (extensionCount == 0)
	{
		return {};
	}

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
	return availableExtensions;
}

std::vector<VkLayerProperties> Application::GetSupportedValidationLayers()
{
	uint32_t count;
	vkEnumerateInstanceLayerProperties(&count, nullptr);

	if (count == 0)
	{
		return {};
	}

	std::vector<VkLayerProperties> properties(count);
	vkEnumerateInstanceLayerProperties(&count, properties.data());
	return properties;
}

namespace 
{
	bool IsExtensionNameEqual(const gsl::czstring name, const VkExtensionProperties& extension)
	{
		return streq(extension.extensionName, name);
	}

	bool IsExtensionSupported(gsl::span<VkExtensionProperties> extensions, gsl::czstring name)
	{
		return std::ranges::any_of(extensions,std::bind_front(IsExtensionNameEqual, name));
	}
}

bool Application::AreAllExtensionsSupported(const gsl::span<gsl::czstring>& extensions)
{
	auto supportedExtensions = GetSupportedInstanceExtensions();

	return std::ranges::all_of(extensions,std::bind_front(IsExtensionSupported, supportedExtensions));
}

bool Application::AreAllDeviceExtensionsSupported(const VkPhysicalDevice device)
{
	auto availableExtensions = GetAvailableDeviceExtensions(device);

	return std::ranges::all_of(m_RequiredDeviceExtensions,std::bind_front(IsExtensionSupported, availableExtensions));
}

#pragma endregion

#pragma region DEVICES_AND_QUEUES

bool Application::IsDeviceSuitable(const VkPhysicalDevice device)
{
	const QueueFamilyIndices indices = FindQueueFamilies(device);

	return indices.IsValid() && AreAllDeviceExtensionsSupported(device);
}

std::vector<VkPhysicalDevice> Application::GetAvailablePhysicalDevices() const
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

std::vector<VkExtensionProperties> Application::GetAvailableDeviceExtensions(VkPhysicalDevice device)
{
	uint32_t extensionCount = 0;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> extensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());
	return extensions;
}

Application::QueueFamilyIndices Application::FindQueueFamilies(const VkPhysicalDevice device) const
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
	result.graphicsFamily = iter - queueFamilies.begin();

	for (uint32_t i = 0; i < queueFamilies.size(); i++)
	{
		VkBool32 has_presentation_support = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &has_presentation_support);
		if (has_presentation_support)
		{
			result.presentationFamily = i;
			break;
		}
	}

	return result;
}

void Application::PickPhysicalDevice()
{
	auto devices = GetAvailablePhysicalDevices();
	std::erase_if(devices, std::not_fn( std::bind_front(&Application::IsDeviceSuitable, this) ));

	if (devices.empty())
	{
		spdlog::error("failed to get physical device list!");
		std::exit(EXIT_FAILURE);
	}

	m_PhysicalDevice = devices[0];

	/*for (const auto& device : devices)
	{	// print the device(s) that can support Vulkan
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(device, &properties);
		spdlog::info(properties.deviceName);
	}*/
	
}

void Application::CreateLogicalDeviceAndQueues()
{
	QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies =
	{
		indices.graphicsFamily.value(),
		indices.presentationFamily.value()
	};
	

	if (!indices.IsValid())
	{
		spdlog::error("failed to get queue families!");
		std::exit(EXIT_FAILURE);
	}

	// Vulkan lets you assign priorities to queues to
	// influence the scheduling of command buffer execution.
	// This is required even if there is only a single queue:
	float queuePriority = 1.0f;
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

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = m_RequiredDeviceExtensions.size();
	createInfo.ppEnabledExtensionNames = m_RequiredDeviceExtensions.data();

	if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
	vkGetDeviceQueue(m_LogicalDevice, indices.presentationFamily.value(), 0, &m_PresentQueue);
}

#pragma endregion

#pragma region PRESENTATION

void Application::CreateSurface()
{
	VkResult result = glfwCreateWindowSurface(m_Instance, m_Window, nullptr, &m_Surface);
	if (result != VK_SUCCESS)
	{
		spdlog::error("failed to create window surface!");
		std::exit(EXIT_FAILURE);
	}
}

Application::SwapChainProperties Application::GetSwapChainProperties(VkPhysicalDevice device) const
{
	SwapChainProperties properties;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &properties.capabilities);

	uint32_t format_count;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &format_count, nullptr);

	properties.formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &format_count, properties.formats.data());

	uint32_t modes_count;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &modes_count, nullptr);

	properties.presentModes.resize(modes_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &modes_count, properties.presentModes.data());

	return properties;
}

namespace 
{
	bool IsRgbaTypeFormat(const VkSurfaceFormatKHR& formatProperties)
	{
		return formatProperties.format == VK_FORMAT_R8G8B8A8_SRGB ||
			   formatProperties.format == VK_FORMAT_B8G8R8A8_SRGB;
	}

	bool IsSrgbColorSpace(const VkSurfaceFormatKHR& formatProperties)
	{
		return formatProperties.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	}

	bool IsCorrectFormat(const VkSurfaceFormatKHR& formatProperties)
	{
		return IsSrgbColorSpace(formatProperties) && IsSrgbColorSpace(formatProperties);
	}

	bool IsMailboxPresentMode(const VkPresentModeKHR& mode)
	{
		return mode == VK_PRESENT_MODE_MAILBOX_KHR;
	}
}

VkSurfaceFormatKHR Application::ChooseSwapSurfaceFromat(gsl::span<VkSurfaceFormatKHR> formats)
{
	if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
	{
		return {VK_FORMAT_R8G8B8A8_SRGB, VK_COLORSPACE_SRGB_NONLINEAR_KHR};
	}

	const auto it = std::ranges::find_if(formats, IsCorrectFormat);
	if (it != formats.end())
	{
		return *it;
	}

	return formats[0];
}

VkPresentModeKHR Application::ChooseSwapPresentMode(gsl::span<VkPresentModeKHR> modes)
{
	if (std::ranges::any_of(modes, IsMailboxPresentMode))
	{
		return VK_PRESENT_MODE_MAILBOX_KHR;
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D Application::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
{
	constexpr uint32_t kInvalidSize = std::numeric_limits<uint32_t>::max();

	if (capabilities.currentExtent.width != kInvalidSize)
	{
		return capabilities.currentExtent;
	}

	glm::ivec2 size;
	glfwGetFramebufferSize(m_Window, &size.x, &size.y);
	VkExtent2D actualExtent =
	{
		static_cast<uint32_t>(size.x),
		static_cast<uint32_t>(size.y),
	};

	actualExtent.width = std::clamp(
		actualExtent.width,
		capabilities.minImageExtent.width,
		capabilities.maxImageExtent.width
	);
	
	actualExtent.height = std::clamp(
		actualExtent.height,
		capabilities.minImageExtent.height,
		capabilities.maxImageExtent.height
	);

	return actualExtent;
}

uint32_t Application::ChooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities)
{
	uint32_t imageCount = capabilities.minImageCount + 1;
	if (capabilities.maxImageCount > 0 && capabilities.maxImageCount < imageCount)
	{
		imageCount = capabilities.maxImageCount;
	}

	return imageCount;
}

void Application::CreateSwapChain()
{
	SwapChainProperties properties = GetSwapChainProperties(m_PhysicalDevice);

	m_SurfaceFormat = ChooseSwapSurfaceFromat(properties.formats);
	m_PresentMode = ChooseSwapPresentMode(properties.presentModes);
	m_Extent = ChooseSwapExtent(properties.capabilities);
	uint32_t image_count = ChooseSwapImageCount(properties.capabilities);

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = m_Surface;
	info.minImageCount = image_count;
	info.imageFormat = m_SurfaceFormat.format;
	info.imageColorSpace = m_SurfaceFormat.colorSpace;
	info.imageExtent = m_Extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	info.presentMode = m_PresentMode;
	info.preTransform = properties.capabilities.currentTransform;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.clipped = VK_TRUE;
	info.oldSwapchain = VK_NULL_HANDLE;

	const QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

	if (indices.graphicsFamily != indices.presentationFamily)
	{
		const std::array<uint32_t, 2> family_indices =
		{
			indices.graphicsFamily.value(),
			indices.presentationFamily.value(),
		};

		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = family_indices.size();
		info.pQueueFamilyIndices = family_indices.data();
	}
	else
	{
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	VkResult result = vkCreateSwapchainKHR(m_LogicalDevice, &info, nullptr, &m_SwapChain);
	if (result != VK_SUCCESS)
	{
		std::exit(EXIT_FAILURE);
	}

	m_SwapChainImages.resize(image_count);
	vkGetSwapchainImagesKHR(m_LogicalDevice, m_SwapChain, &image_count, m_SwapChainImages.data());
}

#pragma endregion
