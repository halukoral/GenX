#include "Application.h"
#include "Core.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

//#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

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

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugReport(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	std::cerr << "[vulkan] Debug report from ObjectType: " << objectType << "\nMessage: " << pMessage << "\n\n";
	return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

namespace
{
	VkAllocationCallbacks*	 g_Allocator = nullptr;
	VkInstance               g_Instance = VK_NULL_HANDLE;
	VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
	VkDevice                 g_Device = VK_NULL_HANDLE;
	uint32_t                 g_QueueFamily = (uint32_t)-1;
	VkQueue                  g_Queue = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
	VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
	VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

	ImGui_ImplVulkanH_Window g_MainWindowData;
	int                      g_MinImageCount = 2;
	bool                     g_SwapChainRebuild = false;

	// Per-frame-in-flight
	std::vector<std::vector<VkCommandBuffer>> s_AllocatedCommandBuffers;
	std::vector<std::vector<std::function<void()>>> s_ResourceFreeQueue;

	// Unlike g_MainWindowData.FrameIndex, this is not the swapchain image index
	// and is always guaranteed to increase (e.g. 0, 1, 2, 0, 1, 2)
	uint32_t s_CurrentFrameIndex = 0;
	
	void SetupVulkan(const char** extensions, uint32_t extensions_count)
	{
		VkResult err;

		// Create Vulkan Instance
		{
			VkInstanceCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			create_info.enabledExtensionCount = extensions_count;
			create_info.ppEnabledExtensionNames = extensions;
	#ifdef IMGUI_VULKAN_DEBUG_REPORT
			// Enabling validation layers
			const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
			create_info.enabledLayerCount = 1;
			create_info.ppEnabledLayerNames = layers;

			// Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
			const char** extensions_ext = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
			memcpy(extensions_ext, extensions, extensions_count * sizeof(const char*));
			extensions_ext[extensions_count] = "VK_EXT_debug_report";
			create_info.enabledExtensionCount = extensions_count + 1;
			create_info.ppEnabledExtensionNames = extensions_ext;

			// Create Vulkan Instance
			err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
			CheckVkResult(err);
			free(extensions_ext);

			// Get the function pointer (required for any extensions)
			auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
			IM_ASSERT(vkCreateDebugReportCallbackEXT != nullptr);

			// Setup the debug report callback
			VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
			debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
			debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
			debug_report_ci.pfnCallback = DebugReport;
			debug_report_ci.pUserData = nullptr;
			err = vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
			CheckVkResult(err);
	#else
			// Create Vulkan Instance without any debug feature
			err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
			CheckVkResult(err);
			IM_UNUSED(g_DebugReport);
	#endif
		}

		// Select GPU
		{
			uint32_t gpu_count;
			err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, nullptr);
			CheckVkResult(err);
			IM_ASSERT(gpu_count > 0);

			VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
			err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus);
			CheckVkResult(err);

			// If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
			// most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
			// dedicated GPUs) is out of scope of this sample.
			int use_gpu = 0;
			for (int i = 0; i < (int)gpu_count; i++)
			{
				VkPhysicalDeviceProperties properties;
				vkGetPhysicalDeviceProperties(gpus[i], &properties);
				if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
				{
					use_gpu = i;
					break;
				}
			}

			g_PhysicalDevice = gpus[use_gpu];
			free(gpus);
		}

		// Select graphics queue family
		{
			uint32_t count;
			vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, nullptr);
			VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
			vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
			for (uint32_t i = 0; i < count; i++)
				if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					g_QueueFamily = i;
					break;
				}
			free(queues);
			IM_ASSERT(g_QueueFamily != (uint32_t)-1);
		}

		// Create Logical Device (with 1 queue)
		{
			int device_extension_count = 1;
			const char* device_extensions[] = { "VK_KHR_swapchain" };
			constexpr float queue_priority[] = { 1.0f };
			VkDeviceQueueCreateInfo queue_info[1] = {};
			queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info[0].queueFamilyIndex = g_QueueFamily;
			queue_info[0].queueCount = 1;
			queue_info[0].pQueuePriorities = queue_priority;
			VkDeviceCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			create_info.queueCreateInfoCount = std::size(queue_info);
			create_info.pQueueCreateInfos = queue_info;
			create_info.enabledExtensionCount = device_extension_count;
			create_info.ppEnabledExtensionNames = device_extensions;
			err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
			CheckVkResult(err);
			vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
		}

		// Create Descriptor Pool
		{
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
			CheckVkResult(err);
		}
	}

	// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
	// Your real engine/app may not use them.
	void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
	{
		wd->Surface = surface;

		// Check for WSI support
		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
		if (res != VK_TRUE)
		{
			std::cerr << "Error no WSI support on physical device 0\n";
			exit(-1);
		}

		// Select Surface Format
		constexpr VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
		constexpr VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

		// Select Present Mode
	#ifdef IMGUI_UNLIMITED_FRAME_RATE
		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
	#else
		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
	#endif
		wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
		//printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

		// Create SwapChain, RenderPass, Framebuffer, etc.
		IM_ASSERT(g_MinImageCount >= 2);
		ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
	}

	void CleanupVulkan()
	{
		vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

	#ifdef IMGUI_VULKAN_DEBUG_REPORT
		// Remove the debug report callback
		auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
		vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
	#endif // IMGUI_VULKAN_DEBUG_REPORT

		vkDestroyDevice(g_Device, g_Allocator);
		vkDestroyInstance(g_Instance, g_Allocator);
	}

	void CleanupVulkanWindow()
	{
		ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
	}

	void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
	{
		VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
		VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		VkResult err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);

		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		{
			g_SwapChainRebuild = true;
			return;
		}
		CheckVkResult(err);

		s_CurrentFrameIndex = (s_CurrentFrameIndex + 1) % g_MainWindowData.ImageCount;

		const ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
		{
			err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
			CheckVkResult(err);

			err = vkResetFences(g_Device, 1, &fd->Fence);
			CheckVkResult(err);
		}
		
		{
			// Free resources in queue
			for (auto& func : s_ResourceFreeQueue[s_CurrentFrameIndex])
				func();
			s_ResourceFreeQueue[s_CurrentFrameIndex].clear();
		}

		{
			// Free command buffers allocated by Application::GetCommandBuffer
			// These use g_MainWindowData.FrameIndex and not s_CurrentFrameIndex because they're tied to the swapchain image index
			auto& allocatedCommandBuffers = s_AllocatedCommandBuffers[wd->FrameIndex];
			if (!allocatedCommandBuffers.empty())
			{
				vkFreeCommandBuffers(g_Device, fd->CommandPool, (uint32_t)allocatedCommandBuffers.size(), allocatedCommandBuffers.data());
				allocatedCommandBuffers.clear();
			}

			err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
			CheckVkResult(err);
			VkCommandBufferBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
			err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
			CheckVkResult(err);
		}
		{
			VkRenderPassBeginInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			info.renderPass = wd->RenderPass;
			info.framebuffer = fd->Framebuffer;
			info.renderArea.extent.width = wd->Width;
			info.renderArea.extent.height = wd->Height;
			info.clearValueCount = 1;
			info.pClearValues = &wd->ClearValue;
			vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
		}

		// Record dear imgui primitives into command buffer
		ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

		// Submit command buffer
		vkCmdEndRenderPass(fd->CommandBuffer);
		{
			VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			VkSubmitInfo info = {};
			info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			info.waitSemaphoreCount = 1;
			info.pWaitSemaphores = &image_acquired_semaphore;
			info.pWaitDstStageMask = &wait_stage;
			info.commandBufferCount = 1;
			info.pCommandBuffers = &fd->CommandBuffer;
			info.signalSemaphoreCount = 1;
			info.pSignalSemaphores = &render_complete_semaphore;

			err = vkEndCommandBuffer(fd->CommandBuffer);
			CheckVkResult(err);
			err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
			CheckVkResult(err);
		}
	}

	void FramePresent(ImGui_ImplVulkanH_Window* wd)
	{
		if (g_SwapChainRebuild)
			return;
		const VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
		VkPresentInfoKHR info = {};
		info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &render_complete_semaphore;
		info.swapchainCount = 1;
		info.pSwapchains = &wd->Swapchain;
		info.pImageIndices = &wd->FrameIndex;
		const VkResult err = vkQueuePresentKHR(g_Queue, &info);
		if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
		{
			g_SwapChainRebuild = true;
			return;
		}
		CheckVkResult(err);
		wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
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
	m_WindowHandle = glfwCreateWindow((int)m_Spec.Width, (int)m_Spec.Height, m_Spec.Name.c_str(), nullptr, nullptr);

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		std::cerr << "GLFW: Vulkan not supported!\n";
		return;
	}
	uint32_t extensions_count = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
	SetupVulkan(extensions, extensions_count);

	// Create Window Surface
	VkSurfaceKHR surface;
	VkResult err = glfwCreateWindowSurface(g_Instance, m_WindowHandle, g_Allocator, &surface);
	CheckVkResult(err);

	// Create Framebuffers
	int w, h;
	glfwGetFramebufferSize(m_WindowHandle, &w, &h);
	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
	SetupVulkanWindow(wd, surface, w, h);

	s_AllocatedCommandBuffers.resize(wd->ImageCount);
	s_ResourceFreeQueue.resize(wd->ImageCount);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(m_WindowHandle, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = g_Instance;
	init_info.PhysicalDevice = g_PhysicalDevice;
	init_info.Device = g_Device;
	init_info.QueueFamily = g_QueueFamily;
	init_info.Queue = g_Queue;
	init_info.PipelineCache = g_PipelineCache;
	init_info.DescriptorPool = g_DescriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = g_MinImageCount;
	init_info.ImageCount = wd->ImageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = g_Allocator;
	init_info.CheckVkResultFn = CheckVkResult;
	init_info.RenderPass = wd->RenderPass;
	ImGui_ImplVulkan_Init(&init_info);

	// Load default font
	ImFontConfig fontConfig;
	fontConfig.FontDataOwnedByAtlas = false;

	// Upload Fonts
	{
		// Use any command queue
		VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
		VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

		err = vkResetCommandPool(g_Device, command_pool, 0);
		CheckVkResult(err);
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		err = vkBeginCommandBuffer(command_buffer, &begin_info);
		CheckVkResult(err);

		ImGui_ImplVulkan_CreateFontsTexture();

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &command_buffer;
		err = vkEndCommandBuffer(command_buffer);
		CheckVkResult(err);
		err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
		CheckVkResult(err);

		err = vkDeviceWaitIdle(g_Device);
		CheckVkResult(err);
	}
}

void Application::Shutdown()
{
	for (const auto& layer : m_LayerStack)
		layer->OnDetach();

	m_LayerStack.clear();

	// Cleanup
	const VkResult err = vkDeviceWaitIdle(g_Device);
	CheckVkResult(err);

	// Free resources in queue
	for (auto& queue : s_ResourceFreeQueue)
	{
		for (auto& func : queue)
			func();
	}
	s_ResourceFreeQueue.clear();

	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	CleanupVulkanWindow();
	CleanupVulkan();

	glfwDestroyWindow(m_WindowHandle);
	glfwTerminate();

	g_ApplicationRunning = false;
}

void Application::Run()
{
	m_Running = true;

	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
	constexpr ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	const ImGuiIO& io = ImGui::GetIO();
	
	// Main loop
	while (!glfwWindowShouldClose(m_WindowHandle) && m_Running)
	{
		glfwPollEvents();
		
		for (const auto& layer : m_LayerStack)
			layer->OnUpdate(m_TimeStep);

		////////////////////////////////////
		// Render

		// Resize swap chain?
		if (g_SwapChainRebuild)
		{
			int width, height;
			glfwGetFramebufferSize(m_WindowHandle, &width, &height);
			if (width > 0 && height > 0)
			{
				ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
				ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
				g_MainWindowData.FrameIndex = 0;

				// Clear allocated command buffers from here since entire pool is destroyed
				s_AllocatedCommandBuffers.clear();
				s_AllocatedCommandBuffers.resize(g_MainWindowData.ImageCount);

				g_SwapChainRebuild = false;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		{
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
			// because it would be confusing to have two docking targets within each others.
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
			if (m_MenubarCallback)
				window_flags |= ImGuiWindowFlags_MenuBar;

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
			// and handle the pass-thru hole, so we ask Begin() to not render a background.
			if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
				window_flags |= ImGuiWindowFlags_NoBackground;

			// Important: note that we proceed even if Begin() returns false (aka window is collapsed).
			// This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
			// all active windows docked into it will lose their parent and become undocked.
			// We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
			// any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			ImGui::Begin("DockSpace Demo", nullptr, window_flags);
			ImGui::PopStyleVar();

			ImGui::PopStyleVar(2);

			// Submit the DockSpace
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				const ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}

			if (m_MenubarCallback)
			{
				if (ImGui::BeginMenuBar())
				{
					m_MenubarCallback();
					ImGui::EndMenuBar();
				}
			}

			for (const auto& layer : m_LayerStack)
				layer->OnUIRender();

			ImGui::End();
		}

		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
		wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
		wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
		wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
		wd->ClearValue.color.float32[3] = clear_color.w;
		if (!main_is_minimized)
			FrameRender(wd, main_draw_data);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// Present Main Platform Window
		if (!main_is_minimized)
			FramePresent(wd);
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

VkInstance Application::GetInstance()
{
	return g_Instance;
}

VkPhysicalDevice Application::GetPhysicalDevice()
{
	return g_PhysicalDevice;
}

VkDevice Application::GetDevice()
{
	return g_Device;
}

VkCommandBuffer Application::GetCommandBuffer(bool begin)
{
	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

	// Use any command queue
	const VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = command_pool;
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer& command_buffer = s_AllocatedCommandBuffers[wd->FrameIndex].emplace_back();
	auto err = vkAllocateCommandBuffers(g_Device, &cmdBufAllocateInfo, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	err = vkBeginCommandBuffer(command_buffer, &begin_info);
	CheckVkResult(err);

	return command_buffer;
}

void Application::FlushCommandBuffer(const VkCommandBuffer commandBuffer)
{
	constexpr uint64_t default_fence_timeout = 100000000000;

	VkSubmitInfo end_info = {};
	end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &commandBuffer;
	auto err = vkEndCommandBuffer(commandBuffer);
	CheckVkResult(err);

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	VkFence fence;
	err = vkCreateFence(g_Device, &fenceCreateInfo, nullptr, &fence);
	CheckVkResult(err);

	err = vkQueueSubmit(g_Queue, 1, &end_info, fence);
	CheckVkResult(err);

	err = vkWaitForFences(g_Device, 1, &fence, VK_TRUE, default_fence_timeout);
	CheckVkResult(err);

	vkDestroyFence(g_Device, fence, nullptr);
}


void Application::SubmitResourceFree(std::function<void()>&& func)
{
	s_ResourceFreeQueue[s_CurrentFrameIndex].emplace_back(func);
}