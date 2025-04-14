#include "Application.h"
#include "Core.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <algorithm>
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <vulkan/vulkan.h>

extern bool g_ApplicationRunning;
static Application* s_Instance = nullptr;

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

static VKAPI_ATTR VkBool32 VKAPI_CALL ValidationCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severity,
		VkDebugUtilsMessageTypeFlagsEXT type,
		const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
		void* user_data)
{
	if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
	{
		std::cout << "Vulkan Validation: " << callback_data->pMessage << '\n';
		//spdlog::warn("Vulkan Validation: {}", callback_data->pMessage);
	}
	else
	{
		std::cout << "Vulkan Error: " << callback_data->pMessage << '\n';
		//spdlog::error("Vulkan Error: {}", callback_data->pMessage);
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
	void CheckVkResult(const VkResult err)
	{
		if (err == 0)
			return;

		std::cerr << "[vulkan] Error: VkResult = " << err << '\n';

		if (err < 0)
			abort();
	}

	void GlfwErrorCallback(int error, const char* description)
	{
		std::cerr << "Glfw Error " << error << description << '\n';
	}
}

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
		std::cerr << "Could not initialize GLFW!\n";
		return;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_Window = glfwCreateWindow((int)m_Spec.Width, (int)m_Spec.Height, m_Spec.Name.c_str(), nullptr, nullptr);

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		std::cerr << "GLFW: Vulkan not supported!\n";
		return;
	}

	if (!InitVulkan())
	{
		std::cerr << "Couldn't initialize VULKAN!\n";
		return;
	}
}

bool Application::InitVulkan()
{
	CreateInstance();
	SetupDebugMessenger();
	return true;
}

void Application::Shutdown()
{
	for (const auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();

	////////////////////////////////
	if (EnableValidationLayers)
	{
		vkDestroyDebugUtilsMessengerEXT(m_Instance, debugMessenger, nullptr);
	}
	
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
	std::uint32_t glfwExtensionCount = 0;
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
	std::uint32_t extensionCount = 0;
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
	std::uint32_t count;
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

	bool IsLayerNameEqual(gsl::czstring name, const VkLayerProperties& properties) {
		return streq(properties.layerName, name);
	}

	bool IsLayerSupported(gsl::span<VkLayerProperties> layers, gsl::czstring name) {
		return std::ranges::any_of(layers, std::bind_front(IsLayerNameEqual, name));
	}
}

bool Application::AreAllExtensionsSupported(const gsl::span<gsl::czstring>& extensions)
{
	auto supportedExtensions = GetSupportedInstanceExtensions();

	return std::ranges::all_of(extensions,std::bind_front(IsExtensionSupported, supportedExtensions));
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
	VkResult result = vkCreateDebugUtilsMessengerEXT(m_Instance, &info, nullptr, &debugMessenger);
	if (result != VK_SUCCESS)
	{
		std::cerr << "failed to create debug messenger!\n";
		//spdlog::error("Cannot create debug messenger");
		return;
	}
}