#include "ImguiRenderer.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

ImGuiRenderer::ImGuiRenderer(Device* dev, Window* win, RenderPass* rp, VkCommandPool* cmdPool, uint32_t minImgCount,
	uint32_t imgCount): m_Device(dev), m_Window(win), m_RenderPass(rp), m_CommandPool(cmdPool), m_MinImageCount(minImgCount), m_ImageCount(imgCount)
{
	InitImGui();
}

ImGuiRenderer::~ImGuiRenderer()
{
	Cleanup();
}

void ImGuiRenderer::NewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void ImGuiRenderer::Render(VkCommandBuffer commandBuffer)
{
	ImGui::Begin("Vulkan Triangle Info");
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 
				1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::Text("Vulkan Renderer");
	ImGui::Text("Triangle vertices: 3");
	ImGui::End();

	// Render
	ImGui::Render();
	ImDrawData* drawData = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffer);
}

void ImGuiRenderer::HandleResize() const
{
	ImGui_ImplVulkan_SetMinImageCount(m_MinImageCount);
}

void ImGuiRenderer::InitImGui()
{
	// Descriptor pool ImGui için
	VkDescriptorPoolSize poolSizes[] = {
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

	VkDescriptorPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolInfo.maxSets = 1000 * IM_ARRAYSIZE(poolSizes);
	poolInfo.poolSizeCount = (uint32_t)IM_ARRAYSIZE(poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	if (vkCreateDescriptorPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
	{
		throw std::runtime_error("ImGui descriptor pool oluşturulamadı!");
	}

	// ImGui context oluştur
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); 
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	// Style ayarla
	ImGui::StyleColorsDark();

	// Platform/Renderer backends kurulum
	ImGui_ImplGlfw_InitForVulkan(m_Window->GetWindow(), true);
        
	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = m_Device->GetInstance();
	initInfo.PhysicalDevice = m_Device->GetPhysicalDevice();
	initInfo.Device = m_Device->GetLogicalDevice();
	initInfo.QueueFamily = m_Device->FindQueueFamilies(m_Device->GetPhysicalDevice()).graphicsFamily;
	initInfo.Queue = m_Device->GetGraphicsQueue();
	initInfo.RenderPass = m_RenderPass->GetRenderPass();
	initInfo.PipelineCache = VK_NULL_HANDLE;
	initInfo.DescriptorPool = m_DescriptorPool;
	initInfo.Subpass = 0;
	initInfo.MinImageCount = m_MinImageCount;
	initInfo.ImageCount = m_ImageCount;
	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	initInfo.Allocator = nullptr;
	initInfo.CheckVkResultFn = CheckVkResult;

	ImGui_ImplVulkan_Init(&initInfo);
}

VkCommandBuffer ImGuiRenderer::BeginSingleTimeCommands() const
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = *m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_Device->GetLogicalDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);
	return commandBuffer;
}

void ImGuiRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer) const
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_Device->GetGraphicsQueue());

	vkFreeCommandBuffers(m_Device->GetLogicalDevice(), *m_CommandPool, 1, &commandBuffer);
}

void ImGuiRenderer::CheckVkResult(VkResult err)
{
	if (err == 0) return;
	std::cerr << "[vulkan] Error: VkResult = " << err << std::endl;
	if (err < 0) abort();
}

void ImGuiRenderer::Cleanup() const
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	vkDestroyDescriptorPool(m_Device->GetLogicalDevice(), m_DescriptorPool, nullptr);
}
