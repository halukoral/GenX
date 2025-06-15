#pragma once

#include "pch.h"
#include "Descriptor.h"
#include "Device.h"
#include "Image.h"
#include "ImguiRenderer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "Model.h"

class Renderer
{
public:
	Renderer() = default;
	Renderer(std::shared_ptr<Window> window) : m_Window(window) { }

	void InitVulkan();
	void Cleanup();

	void DrawFrame();
	void LoadModel(const std::string& modelPath);

	const std::unique_ptr<Device>& GetDevice() const { return m_Device; }
	const std::unique_ptr<RenderPass>& GetSwapChainRenderPass() const { return m_RenderPass; }
	VkCommandBuffer GetCurrentCommandBuffer() const	{ return m_CommandBuffers[m_CurrentFrame]; }
	
private:
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateCommandBuffers();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
	void CreateSyncObjects();

private:
	std::shared_ptr<Window> m_Window;
	std::unique_ptr<Device> m_Device;
	std::unique_ptr<SwapChain> m_SwapChain;
	std::unique_ptr<Image> m_Image;
	std::unique_ptr<RenderPass> m_RenderPass;
	std::unique_ptr<Pipeline> m_Pipeline;
	std::unique_ptr<Descriptor> m_Descriptor;
	std::unique_ptr<ImGuiRenderer> imguiRenderer;
	std::unique_ptr<Model> m_Model;
	
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	size_t m_CurrentFrame = 0;
	
	const int MAX_FRAMES_IN_FLIGHT = 2;
};