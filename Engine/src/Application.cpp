#include "Application.h"
#include "Core.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <ranges>

#include "Logger.h"
#include "TimeStep.h"
#include "Event/ApplicationEvent.h"

extern bool g_ApplicationRunning;
static Application* s_Instance = nullptr;

static uint32_t                 g_MinImageCount = 2;

static void CheckVkResult(const VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

struct SimplePushConstantData
{
	glm::mat2 Transform{1.f};
	glm::vec2 Offset;
	alignas(16) glm::vec3 Color;
};

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

void Application::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);

	for (const auto& it : std::ranges::reverse_view(m_LayerStack))
	{
		if (e.GetHandled() == true)
		{
			break;
		}
		it->OnEvent(e);
	}
}

bool Application::OnWindowClose(WindowCloseEvent& e)
{
	m_Running = false;
	return false;
}

void Application::Init()
{
	LOG_INFO("Application starting!");
	if (!glfwVulkanSupported())
	{
		return;
	}

	m_Window->SetEventCallback(GX_BIND(Application::OnEvent));
	//m_Window->DisableCursor();
	
	m_Renderer->InitVulkan();
}

void Application::Shutdown()
{
	LOG_INFO("Application shutdown!");
	for (const auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();
	m_Renderer->Cleanup();
	g_ApplicationRunning = false;
}

void Application::Run()
{
	m_Running = true;

	// Main loop
	while (!glfwWindowShouldClose(m_Window->GetWindow()) && m_Running)
	{
		glfwPollEvents();
		
		for (const auto& layer : m_LayerStack)
			layer->OnUpdate(m_TimeStep);

		////////////////////////////////////
		// Render
		m_Renderer->DrawFrame();
		////////////////////////////////////

		const float time = GetTime();
		m_FrameTime = time - m_LastFrameTime;
		m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
		m_LastFrameTime = time;
	}
	vkDeviceWaitIdle(m_Renderer->GetDevice()->GetLogicalDevice());
}

void Application::Close()
{
	m_Running = false;
}

float Application::GetTime()
{
	return (float)glfwGetTime();
}
