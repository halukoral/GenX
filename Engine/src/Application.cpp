#include "Application.h"
#include "Core.h"

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <glm/common.hpp>
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
	PickPhysicalDevice();
	CreateLogicalDeviceAndQueues();
	CreateSurface();
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

#pragma endregion

#pragma region DEVICES_AND_QUEUES

bool Application::IsDeviceSuitable(const VkPhysicalDevice device)
{
	const QueueFamilyIndices indices = FindQueueFamilies(device);

	return indices.IsValid();
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

Application::QueueFamilyIndices Application::FindQueueFamilies(const VkPhysicalDevice device)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	const auto iter = std::ranges::find_if(queueFamilies, [](const VkQueueFamilyProperties& property)
	{
		return property.queueFlags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT);
	});

	QueueFamilyIndices result;
	result.graphicsFamily = iter - queueFamilies.begin();
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

	if (!indices.IsValid())
	{
		spdlog::error("failed to get queue families!");
		std::exit(EXIT_FAILURE);
	}

	// Vulkan lets you assign priorities to queues to
	// influence the scheduling of command buffer execution.
	// This is required even if there is only a single queue:
	float queuePriority = 1.0f;

	// VkDeviceQueueCreateInfo describes the number of queues we want for a single queue family.
	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	// We also need to specify the set of device features that we’ll be using
	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo deviceCreateInfo{};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
	deviceCreateInfo.enabledExtensionCount = 0;

	VkResult result = vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_LogicalDevice);
	if (result != VK_SUCCESS)
	{
		spdlog::error("failed to create logical device!");
		std::exit(EXIT_FAILURE);
	}

	vkGetDeviceQueue(m_LogicalDevice, indices.graphicsFamily.value(), 0, &m_GraphicsQueue);
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

#pragma endregion
