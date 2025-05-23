#include "Application.h"
#include "Core.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <ranges>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "TimeStep.h"
#include "Event/ApplicationEvent.h"

extern bool g_ApplicationRunning;
static Application* s_Instance = nullptr;

static ImGui_ImplVulkanH_Window g_MainWindowData;
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
	if (!glfwVulkanSupported())
	{
		//spdlog::error("GLFW: Vulkan not supported!");
		return;
	}

	m_Window->SetEventCallback(GX_BIND(Application::OnEvent));
	//m_Window->DisableCursor();
	
	m_Renderer->InitVulkan();

	InitImgui();
}

void Application::InitImgui()
{
	CreateImGuiDescriptorPool();
	
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	
	ImGui::StyleColorsDark();
	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
	
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(m_Window->GetWindow(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_Renderer->GetDevice()->GetInstance();
	init_info.PhysicalDevice = m_Renderer->GetDevice()->GetPhysicalDevice();
	init_info.Device = m_Renderer->GetDevice()->GetLogicalDevice();
	init_info.QueueFamily = m_Renderer->GetDevice()->FindQueueFamilies(m_Renderer->GetDevice()->GetPhysicalDevice()).graphicsFamily;
	init_info.Queue = m_Renderer->GetDevice()->GetGraphicsQueue();
	//init_info.PipelineCache = YOUR_PIPELINE_CACHE;
	init_info.DescriptorPool = m_ImGuiDescriptorPool;
	init_info.Subpass = 0;
	init_info.RenderPass = m_Renderer->GetSwapChainRenderPass()->GetRenderPass();
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = CheckVkResult;
	ImGui_ImplVulkan_Init(&init_info);	
}

void Application::CleanupImGui() const
{
	vkDeviceWaitIdle(m_Renderer->GetDevice()->GetLogicalDevice());
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(m_Renderer->GetDevice()->GetLogicalDevice(), m_ImGuiDescriptorPool, nullptr);
}

void Application::Shutdown()
{
	for (const auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();

	m_Renderer->Cleanup();
	
	CleanupImGui();
	
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

		m_Renderer->DrawFrame();
		////////////////////////////////////
		// Render

			// Start the Dear ImGui frame
			// ImGui_ImplVulkan_NewFrame();
			// ImGui_ImplGlfw_NewFrame();
			// ImGui::NewFrame();
			//
			// ImGui::ShowDemoWindow();
			//
			// ImGui::Render();
			// ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Renderer->GetCurrentCommandBuffer());

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

void Application::CreateImGuiDescriptorPool()
{
	const VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER,                1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,          1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,          1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,   1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,   1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,         1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       1000 },
	};

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets       = 1000 * (uint32_t) std::size(pool_sizes);
	pool_info.poolSizeCount = (uint32_t)std::size(pool_sizes);
	pool_info.pPoolSizes    = pool_sizes;

	if (vkCreateDescriptorPool(m_Renderer->GetDevice()->GetLogicalDevice(), &pool_info, nullptr, &m_ImGuiDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create ImGui descriptor pool!");
	}
}
