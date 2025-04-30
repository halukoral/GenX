#include "Application.h"
#include "Core.h"
#include "Vertex.h"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <spdlog/spdlog.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/gtc/constants.hpp>
#include <ranges>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "TimeStep.h"
#include "Actor/CameraActor.h"
#include "ECS/Components/CameraComponent.h"
#include "Event/ApplicationEvent.h"
#include "Renderer/PointLightSystem.h"
#include "Renderer/RenderSystem.h"

extern bool g_ApplicationRunning;
static Application* s_Instance = nullptr;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static uint32_t                 g_MinImageCount = 2;

namespace std
{
	template<> struct hash<Vertex>
	{
		size_t operator()(Vertex const& vertex) const
		{
			return ((hash<glm::vec3>()(vertex.Position) ^ (hash<glm::vec2>()(vertex.TextCoord) << 1)));
		}
	};
}

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
		spdlog::error("GLFW: Vulkan not supported!");
		return;
	}

	m_Window.SetEventCallback(GX_BIND(Application::OnEvent));

	m_Layer = CreateRef<ApplicationLayer>();
	m_Layer->SetCamera(this);
	PushLayer(m_Layer);
	
	m_Window.DisableCursor();
	
	InitVulkan();

	InitImgui();
}

bool Application::InitVulkan()
{
	// Order matters
	m_Device.Initialize();
	m_Renderer.Initialize();

	m_GlobalPool =
	DescriptorPool::Builder(m_Device)
		.SetMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
		.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
		.Build();
	LoadGameObjects();
	
	return true;
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
	ImGui_ImplGlfw_InitForVulkan(m_Window.GetWindow(), true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_Device.GetInstance();
	init_info.PhysicalDevice = m_Device.GetPhysicalDevice();
	init_info.Device = m_Device.GetLogicalDevice();
	init_info.QueueFamily = m_Device.FindQueueFamilies(m_Device.GetPhysicalDevice()).GraphicsFamily.value();
	init_info.Queue = m_Device.GetGraphicsQueue();
	//init_info.PipelineCache = YOUR_PIPELINE_CACHE;
	init_info.DescriptorPool = m_ImGuiDescriptorPool;
	init_info.Subpass = 0;
	init_info.RenderPass = m_Renderer.GetSwapChainRenderPass();
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = nullptr;
	init_info.CheckVkResultFn = CheckVkResult;
	ImGui_ImplVulkan_Init(&init_info);	
}

void Application::CleanupImGui() const
{
	vkDeviceWaitIdle(m_Device.GetLogicalDevice());
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(m_Device.GetLogicalDevice(), m_ImGuiDescriptorPool, nullptr);
}

void Application::Shutdown()
{
	for (const auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();

	CleanupImGui();
	
	g_ApplicationRunning = false;
}

void Application::Run()
{
	m_Running = true;

	std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
	for (auto& uboBuffer : uboBuffers)
	{
		uboBuffer = std::make_unique<Buffer>(
			m_Device,
			sizeof(GlobalUbo),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		uboBuffer->Map();
	}

	const auto globalSetLayout =
	DescriptorSetLayout::Builder(m_Device)
		.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
		.build();
	
	std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
	for (int i = 0; i < globalDescriptorSets.size(); i++)
	{
		auto bufferInfo = uboBuffers[i]->DescriptorInfo();
		DescriptorWriter(*globalSetLayout, *m_GlobalPool)
			.WriteBuffer(0, &bufferInfo)
			.Build(globalDescriptorSets[i]);
	}

	RenderSystem simpleRenderSystem{
		m_Device,
		m_Renderer.GetSwapChainRenderPass(),
		globalSetLayout->GetDescriptorSetLayout()};
	PointLightSystem pointLightSystem{
		m_Device,
		m_Renderer.GetSwapChainRenderPass(),
		globalSetLayout->GetDescriptorSetLayout()};



	// Main loop
	while (!glfwWindowShouldClose(m_Window.GetWindow()) && m_Running)
	{
		glfwPollEvents();
		
		for (const auto& layer : m_LayerStack)
			layer->OnUpdate(m_TimeStep);

		m_CameraActor.OnUpdate(m_TimeStep);
		////////////////////////////////////
		// Render
		if (const auto commandBuffer = m_Renderer.BeginFrame())
		{
			const int frameIndex = m_Renderer.GetFrameIndex();
			FrameInfo frameInfo
			{
				frameIndex,
				m_FrameTime,
				commandBuffer,
				m_CameraActor,
				globalDescriptorSets[frameIndex],
				m_GameObjects
			};

			// update
			GlobalUbo ubo{};
			ubo.Projection = m_CameraActor.GetProjectionMatrix(CameraType::Perspective);
			ubo.View = m_CameraActor.GetViewMatrix();
			ubo.InverseView = glm::inverse(m_CameraActor.GetViewMatrix());
			pointLightSystem.Update(frameInfo, ubo);
			uboBuffers[frameIndex]->WriteToBuffer(&ubo);
			uboBuffers[frameIndex]->Flush();

			// render
			m_Renderer.BeginSwapChainRenderPass(commandBuffer);

			// order here matters
			simpleRenderSystem.RenderGameObjects(frameInfo);
			pointLightSystem.Render(frameInfo);

			// Start the Dear ImGui frame
			// ImGui_ImplVulkan_NewFrame();
			// ImGui_ImplGlfw_NewFrame();
			// ImGui::NewFrame();
			//
			// ImGui::ShowDemoWindow();
			//
			// ImGui::Render();
			// ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_Renderer.getCurrentCommandBuffer());

			m_Renderer.EndSwapChainRenderPass(commandBuffer);
			m_Renderer.EndFrame();
		}
		////////////////////////////////////

		const float time = GetTime();
		m_FrameTime = time - m_LastFrameTime;
		m_TimeStep = glm::min<float>(m_FrameTime, 0.0333f);
		m_LastFrameTime = time;
	}
	vkDeviceWaitIdle(m_Device.GetLogicalDevice());
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

	if (vkCreateDescriptorPool(m_Device.GetLogicalDevice(), &pool_info, nullptr, &m_ImGuiDescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create ImGui descriptor pool!");
	}
}

void Application::LoadGameObjects()
{
	std::shared_ptr<Model> model = Model::CreateModelFromFile(m_Device, "flat_vase.obj");

	auto flatVase = GameObject::createGameObject();
	flatVase.model = model;
	flatVase.transform.Position = {-.5f, .5f, 0.f};
	flatVase.transform.Scale = {3.f, 1.5f, 3.f};
	m_GameObjects.emplace(flatVase.getId(), std::move(flatVase));

	model = Model::CreateModelFromFile(m_Device, "smooth_vase.obj");
	auto smoothVase = GameObject::createGameObject();
	smoothVase.model = model;
	smoothVase.transform.Position = {.5f, .5f, 0.f};
	smoothVase.transform.Scale = {3.f, 1.5f, 3.f};
	m_GameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

	model = Model::CreateModelFromFile(m_Device, "cube.obj");
	auto floor = GameObject::createGameObject();
	floor.model = model;
	floor.transform.Position = {0.f, .5f, 0.f};
	floor.transform.Scale = {3.f, 1.f, 3.f};
	m_GameObjects.emplace(floor.getId(), std::move(floor));

	const std::vector<glm::vec3> lightColors
	{
	      {1.f, .1f, .1f},
		  {.1f, .1f, 1.f},
		  {.1f, 1.f, .1f},
		  {1.f, 1.f, .1f},
		  {.1f, 1.f, 1.f},
		  {1.f, 1.f, 1.f}
	};

	for (int i = 0; i < lightColors.size(); i++)
	{
		auto pointLight = GameObject::makePointLight(0.2f);
		pointLight.color = lightColors[i];
		auto rotateLight = glm::rotate(
			glm::mat4(1.f),
			(i * glm::two_pi<float>()) / lightColors.size(),
			{0.f, -1.f, 0.f});
		pointLight.transform.Position = glm::vec3(rotateLight * glm::vec4(-1.f, -1.f, -1.f, 1.f));
		m_GameObjects.emplace(pointLight.getId(), std::move(pointLight));
	}
}
