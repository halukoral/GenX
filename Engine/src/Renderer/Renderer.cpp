#include "Renderer.h"

void Renderer::InitVulkan()
{
	m_Device = std::make_unique<Device>(m_Window.get());
	m_SwapChain = std::make_unique<SwapChain>(m_Device.get(), m_Window.get());
	m_Image = std::make_unique<Image>(m_Device.get(), m_SwapChain.get());
	m_RenderPass = std::make_unique<RenderPass>(m_Device.get(), m_SwapChain.get());
	
	CreateFramebuffers();
	CreateCommandPool();
	
	// Load model after command pool is created
	LoadModel("../cube.obj");
	
	m_Pipeline = std::make_unique<Pipeline>(m_Device.get(), m_SwapChain.get(), m_RenderPass.get(), 
											m_Model->GetDescriptorSetLayout());

	imguiRenderer = std::make_unique<ImGuiRenderer>(
		m_Device.get(), 
		m_Window.get(), 
		m_RenderPass.get(),
		&m_CommandPool, 
		2, // minImageCount
		static_cast<uint32_t>(m_SwapChain->GetImages().size()) // imageCount
	);
	
	CreateCommandBuffers();
	CreateSyncObjects();
}

void Renderer::LoadModel(const std::string& modelPath) {
	m_Model = std::make_unique<Model>(m_Device.get(), modelPath, m_CommandPool);
	LOG_INFO("Model loaded: {}", modelPath);
}

void Renderer::CreateFramebuffers()
{
	m_SwapChainFramebuffers.resize(m_Image->GetImageViews().size());

	for (size_t i = 0; i < m_Image->GetImageViews().size(); i++)
	{
		VkImageView attachments[] =
		{
			m_Image->GetImageViews()[i]
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass->GetRenderPass();
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = m_SwapChain->GetExtent().width;
		framebufferInfo.height = m_SwapChain->GetExtent().height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(m_Device->GetLogicalDevice(), &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Framebuffer creation failed!");
		}
		LOG_INFO("Framebuffer created successfully for image index: {}", i);
	}
}

void Renderer::CreateCommandPool()
{
	auto queueFamilyIndices = m_Device->FindQueueFamilies(m_Device->GetPhysicalDevice());

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

	if (vkCreateCommandPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Command pool creation failed!");
	}
	LOG_INFO("Command pool created successfully!");
}

void Renderer::CreateCommandBuffers()
{
	m_CommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t) m_CommandBuffers.size();

	if (vkAllocateCommandBuffers(m_Device->GetLogicalDevice(), &allocInfo, m_CommandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("Command buffer couldn't allocated!");
	}
	LOG_INFO("Command buffers allocated successfully!");
}

void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
{
	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
	{
		throw std::runtime_error("Command buffer couldn't start recording!");
	}

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = m_RenderPass->GetRenderPass();
	renderPassInfo.framebuffer = m_SwapChainFramebuffers[imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = m_SwapChain->GetExtent();

	VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());

	// Draw 3D model instead of triangle
	if (m_Model) {
		m_Model->Draw(commandBuffer, m_Pipeline->GetPipelineLayout(), m_CurrentFrame);
	}
	
	imguiRenderer->Render(commandBuffer);
	
	vkCmdEndRenderPass(commandBuffer);

	if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Command buffer couldn't finish recording!");
	}
}

void Renderer::CreateSyncObjects()
{
	m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		if (vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]) != VK_SUCCESS ||
			vkCreateSemaphore(m_Device->GetLogicalDevice(), &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]) != VK_SUCCESS ||
			vkCreateFence(m_Device->GetLogicalDevice(), &fenceInfo, nullptr, &m_InFlightFences[i]) != VK_SUCCESS)
		{
			throw std::runtime_error("Sync object couldn't created!");
		}
		LOG_INFO("Sync objects created successfully for frame: {}", i);
	}
}

void Renderer::DrawFrame()
{
	imguiRenderer->NewFrame();
	
	vkWaitForFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);
	
	// Update uniform buffer
	if (m_Model) {
		m_Model->UpdateUniformBuffer(m_CurrentFrame, m_SwapChain->GetExtent());
	}
	
	vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_SwapChain->GetSwapChain(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	vkResetCommandBuffer(m_CommandBuffers[m_CurrentFrame], 0);
	RecordCommandBuffer(m_CommandBuffers[m_CurrentFrame], imageIndex);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = {m_ImageAvailableSemaphores[m_CurrentFrame]};
	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];

	VkSemaphore signalSemaphores[] = {m_RenderFinishedSemaphores[m_CurrentFrame]};
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFences[m_CurrentFrame]) != VK_SUCCESS)
	{
		throw std::runtime_error("Draw command buffer couldn't submitted!");
	}

	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = {m_SwapChain->GetSwapChain()};
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);

	m_CurrentFrame = (m_CurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::Cleanup()
{
	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroySemaphore(m_Device->GetLogicalDevice(), m_RenderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(m_Device->GetLogicalDevice(), m_ImageAvailableSemaphores[i], nullptr);
		vkDestroyFence(m_Device->GetLogicalDevice(), m_InFlightFences[i], nullptr);
	}

	vkDestroyCommandPool(m_Device->GetLogicalDevice(), m_CommandPool, nullptr);

	for (auto framebuffer : m_SwapChainFramebuffers)
	{
		vkDestroyFramebuffer(m_Device->GetLogicalDevice(), framebuffer, nullptr);
	}
	
	imguiRenderer.reset();
	m_Model.reset();
	m_Pipeline.reset();
	m_RenderPass.reset();
	m_Image.reset();
	m_SwapChain.reset();
	m_Device.reset();
	m_Window.reset();
}