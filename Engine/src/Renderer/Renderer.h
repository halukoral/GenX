#pragma once

#include "pch.h"
#include "Descriptor.h"
#include "Device.h"
#include "Image.h"
#include "ImguiRenderer.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "SwapChain.h"
#include "Texture.h"
#include "Layers/CameraLayer.h"
#include "Layers/PhysicsLayer.h"

class ModelLayer;

class Renderer
{	
public:
	Renderer() = default;
	Renderer(const std::shared_ptr<Window>& window) : m_Window(window) { }

	void InitVulkan();
	void Cleanup();

	void DrawFrame();

	[[nodiscard]] Device* GetDevice() const { return m_Device.get(); }
	[[nodiscard]] VkCommandBuffer GetCurrentCommandBuffer() const	{ return m_CommandBuffers[m_CurrentFrame]; }

	ModelLayer* GetModelLayer() const { return m_ModelLayer.get(); }
	Descriptor* GetDescriptor() const { return m_Descriptor.get(); }
	
	// Model loading
	void LoadTexture(const std::string& texturePath);
	
private:
	void CreateFramebuffers();
	void CreateCommandPool();
	void CreateDepthResources();
	
	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
	void CreateCommandBuffers();
	void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const;
	void CreateSyncObjects();
	void UpdateUniformBuffer(uint32_t currentFrame) const;

	VkFormat FindDepthFormat() const;
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const;

private:
	std::shared_ptr<Window> m_Window;
	std::unique_ptr<Device> m_Device;
	std::unique_ptr<SwapChain> m_SwapChain;
	std::unique_ptr<Image> m_Image;
	std::unique_ptr<RenderPass> m_RenderPass;
	std::unique_ptr<Pipeline> m_Pipeline;
	std::unique_ptr<Descriptor> m_Descriptor;
	std::unique_ptr<ImGuiRenderer> imguiRenderer;

	std::shared_ptr<CameraLayer> m_CameraLayer;
	std::shared_ptr<ModelLayer> m_ModelLayer;
	std::shared_ptr<PhysicsLayer> m_PhysicsLayer;
	std::unique_ptr<Texture> m_Texture;
	
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;

	VkCommandPool m_CommandPool;
	std::vector<VkCommandBuffer> m_CommandBuffers;
	std::vector<VkSemaphore> m_ImageAvailableSemaphores;
	std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	std::vector<VkFence> m_InFlightFences;

	size_t m_CurrentFrame = 0;

	// Depth buffer
	VkImage m_DepthImage;
	VkDeviceMemory m_DepthImageMemory;
	VkImageView m_DepthImageView;
	
	const int MAX_FRAMES_IN_FLIGHT = 2;
};
