#include "Application.h"
#include "Log.h"
#include "Core.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

extern bool g_ApplicationRunning;
static Application* s_Instance = nullptr;

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
{
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr)
	{
		func(instance, debugMessenger, pAllocator);
	}
}


static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	LOG_ERROR("validation layer: {0}", pCallbackData->pMessage);
	return VK_FALSE;
}

bool CheckValidationLayerSupport()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}

		if (layerFound == false)
		{
			return false;
		}
	}
	
	return true;
}

std::vector<const char*> GetRequiredExtensions()
{
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (EnableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

void CheckVkResult(VkResult err)
{
	if (err == 0)
		return;

	LOG_ERROR("[vulkan] Error: VkResult = {0}", err);

	if (err < 0)
		abort();
}

static void GlfwErrorCallback(int error, const char* description)
{
	LOG_ERROR("GLFW Error ({0}): {1}", error, description);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	if (!InitWindow())
	{
		LOG_ERROR("Could not create window!");
		glfwTerminate();
		return;
	}

	if (!InitVulkan())
	{
		LOG_ERROR("Could not init Vulkan!");
		glfwTerminate();
		return;
	}
}

bool Application::InitWindow()
{
	// Setup GLFW window
	glfwSetErrorCallback(GlfwErrorCallback);
	if (!glfwInit())
	{
		LOG_ERROR("Could not initialize GLFW!");
		return false;
	}

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		LOG_ERROR("GLFW: Vulkan not supported!");
		return false;
	}
	
	/* set a "hint" for the NEXT window created*/
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	/* Vulkan needs no context */
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_WindowHandle = glfwCreateWindow(m_Spec.Width, m_Spec.Height, m_Spec.Name.c_str(), nullptr, nullptr);

	return m_WindowHandle ? true : false;
}

bool Application::InitVulkan()
{
	CreateInstance();
	SetupDebugMessenger();
	
	uint32_t PhysicalDeviceCount = 0;
	vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, nullptr);

	if (PhysicalDeviceCount == 0)
	{
		LOG_ERROR("No Vulkan capable GPU found");
		return false;
	}

	std::vector<VkPhysicalDevice> Devices;
	Devices.resize(PhysicalDeviceCount);
	vkEnumeratePhysicalDevices(m_Instance, &PhysicalDeviceCount, Devices.data());

	LOG_INFO("{0} : Found {1} physical device(s)", __FUNCTION__, PhysicalDeviceCount);

	VkResult Result = glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface);
	if (Result != VK_SUCCESS)
	{
		LOG_ERROR("Could not create Vulkan surface");
		return false;
	}

	return true;
}

void Application::Shutdown()
{
	for (auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();

	if (EnableValidationLayers)
	{
		DestroyDebugUtilsMessengerEXT(m_Instance, debugMessenger, nullptr);
	}
	
	vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
	vkDestroyInstance(m_Instance, nullptr);

	glfwDestroyWindow(m_WindowHandle);
	glfwTerminate();

	g_ApplicationRunning = false;
}

void Application::CreateInstance()
{
	if (EnableValidationLayers && !CheckValidationLayerSupport())
	{
		LOG_ERROR("Validation layers requested, but not available!");
	}
	
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	auto extensions = GetRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	if (EnableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		PopulateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
	}
	else
	{
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
	}
	
	VkResult Result = vkCreateInstance(&createInfo, nullptr, &m_Instance);
	if (Result != VK_SUCCESS)
	{
		LOG_ERROR("Could not create Vulkan instance");
	}
}

void Application::SetupDebugMessenger()
{
	if constexpr (!EnableValidationLayers)
		return;

	VkDebugUtilsMessengerCreateInfoEXT createInfo;
	PopulateDebugMessengerCreateInfo(createInfo);

	if (CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
	{
		LOG_ERROR("failed to set up debug messenger!");
	}
}

void Application::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debugCallback;
}

void Application::Run()
{
	m_Running = true;

	// Main loop
	while (!glfwWindowShouldClose(m_WindowHandle) && m_Running)
	{
		glfwPollEvents();

		for (auto& layer : m_LayerStack)
			layer->OnUpdate(m_TimeStep);

		////////////////////////////////////
		// Render



		////////////////////////////////////

		float time = GetTime();
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