#include "Application.h"
#include "Core.h"

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/common.hpp>
#include <vulkan/vulkan.h>

extern bool g_ApplicationRunning;
static Application* s_Instance = nullptr;

void CheckVkResult(const VkResult err)
{
	if (err == 0)
		return;

	std::cerr << "[vulkan] Error: VkResult = " << err << '\n';

	if (err < 0)
		abort();
}

namespace
{
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
	return true;
}

void Application::CreateInstance()
{
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

	// Get Suggested Extensions
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	createInfo.enabledLayerCount = 0;

	if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create instance!");
	}
}

void Application::Shutdown()
{
	for (const auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();

	////////////////////////////////
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
