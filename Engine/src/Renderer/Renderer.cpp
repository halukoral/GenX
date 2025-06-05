#include "Renderer.h"

#include "Application.h"
#include "Layers/ModelLayer.h"
#include "Layers/PhysicsLayer.h"

void Renderer::InitVulkan()
{
	m_Device = std::make_unique<Device>(m_Window.get());
	m_SwapChain = std::make_unique<SwapChain>(m_Device.get(), m_Window.get());
	m_Image = std::make_unique<Image>(m_Device.get(), m_SwapChain.get());
	m_RenderPass = std::make_unique<RenderPass>(m_Device.get(), m_SwapChain.get());
	m_Descriptor = std::make_unique<Descriptor>(m_Device.get(), MAX_FRAMES_IN_FLIGHT);
	m_Pipeline = std::make_unique<Pipeline>(m_Device.get(), m_SwapChain.get(), m_RenderPass.get(), m_Descriptor.get());

	imguiRenderer = std::make_unique<ImGuiRenderer>(
		m_Device.get(), 
		m_Window.get(), 
		m_RenderPass.get(),
		&m_CommandPool, 
		2, // minImageCount
		static_cast<uint32_t>(m_SwapChain->GetImages().size()) // imageCount
	);

	CreateDepthResources(); // Create Depth buffer
	CreateFramebuffers();
	CreateCommandPool();
	CreateCommandBuffers();
	CreateSyncObjects();

	m_CameraLayer = std::make_shared<CameraLayer>();
	Application::Get().PushLayer(m_CameraLayer);

	m_ModelLayer = std::make_shared<ModelLayer>();
	m_ModelLayer->SetDevice(m_Device.get());
	m_ModelLayer->SetDescriptor(m_Descriptor.get());
	m_ModelLayer->SetRenderPipeline(m_Pipeline->GetPipeline(), m_Pipeline->GetPipelineLayout());
	Application::Get().PushLayer(m_ModelLayer);
    
	// Create some models
	//auto viking = m_ModelLayer->CreateModel("../cube.obj", glm::vec3(0, 0, 2));
	//LoadTexture("../viking_room.png");

	m_PhysicsLayer = std::make_shared<PhysicsLayer>();
	m_PhysicsLayer->SetModelLayer(m_ModelLayer.get());
	m_PhysicsLayer->EnableDemo(false);
	Application::Get().PushLayer(m_PhysicsLayer);
}

void Renderer::LoadTexture(const std::string& texturePath)
{
	m_Texture = std::make_unique<Texture>(m_Device.get(), texturePath);
	m_Descriptor->UpdateTextureDescriptor(m_Texture.get());

	LOG_INFO("Texture loaded: {}", texturePath);
}

void Renderer::CreateDepthResources()
{
    VkFormat depthFormat = FindDepthFormat();
    
    CreateImage(m_SwapChain->GetExtent().width, m_SwapChain->GetExtent().height, depthFormat, 
               VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_DepthImage, m_DepthImageMemory);

    m_DepthImageView = CreateImageView(m_DepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void Renderer::UpdateUniformBuffer(const uint32_t currentFrame) const
{
	if (!m_CameraLayer) return;
    
	const auto& cameraData = m_CameraLayer->GetCameraData();
    
	UniformBufferObject ubo{};
	ubo.View = cameraData.view;
	ubo.Proj = cameraData.projection;
    
	m_Descriptor->UpdateUniformBuffer(currentFrame, ubo);
}

void Renderer::CreateFramebuffers()
{
	m_SwapChainFramebuffers.resize(m_Image->GetImageViews().size());

	for (size_t i = 0; i < m_Image->GetImageViews().size(); i++)
	{
		std::array<VkImageView, 2> attachments =
		{
			m_Image->GetImageViews()[i],
			m_DepthImageView  // Add Depth image view
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = m_RenderPass->GetRenderPass();
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // 2 attachment
		framebufferInfo.pAttachments = attachments.data();
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
	poolInfo.queueFamilyIndex = queueFamilyIndices.GraphicsFamily;

	if (vkCreateCommandPool(m_Device->GetLogicalDevice(), &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS)
	{
		throw std::runtime_error("Command pool creation failed!");
	}
	LOG_INFO("Command pool created successfully!");
}

void Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
							VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(m_Device->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
	{
		throw std::runtime_error("Buffer couldn't created!");
	}
	LOG_INFO("Buffer created successfully!");

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(m_Device->GetLogicalDevice(), buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = m_Device->FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(m_Device->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
	{
		throw std::runtime_error("Buffer memory couldn't allocated!");
	}
	LOG_INFO("Buffer memory allocated successfully!");
	
	vkBindBufferMemory(m_Device->GetLogicalDevice(), buffer, bufferMemory, 0);
}

void Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = m_CommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(m_Device->GetLogicalDevice(), &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(m_Device->GetGraphicsQueue());

	vkFreeCommandBuffers(m_Device->GetLogicalDevice(), m_CommandPool, 1, &commandBuffer);
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

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.2f, 0.3f, 0.3f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipeline());

    // Uniform buffer bind et
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline->GetPipelineLayout(), 
                           0, 1, &m_Descriptor->GetDescriptorSet(m_CurrentFrame), 0, nullptr);

    // Render 3D model
	if (m_ModelLayer && m_CameraLayer)
	{
		auto& cameraData = m_CameraLayer->GetCameraData();
        
		LOG_DEBUG("Rendering models - Camera pos: ({}, {}, {})", 
				 cameraData.position.x, cameraData.position.y, cameraData.position.z);
        
		m_ModelLayer->Render(commandBuffer, 
						   cameraData.position,
						   cameraData.view, 
						   cameraData.projection,
						   m_CurrentFrame);
	} else {
		LOG_WARN("ModelLayer or CameraLayer is null!");
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
	vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

	uint32_t imageIndex;
	vkAcquireNextImageKHR(m_Device->GetLogicalDevice(), m_SwapChain->GetSwapChain(), UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame], VK_NULL_HANDLE, &imageIndex);

	// Update Uniform Buffer
	UpdateUniformBuffer(m_CurrentFrame);
    
	vkResetFences(m_Device->GetLogicalDevice(), 1, &m_InFlightFences[m_CurrentFrame]);

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

	const VkSwapchainKHR swapChains[] = {m_SwapChain->GetSwapChain()};
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

	// Depth resources cleanup
	vkDestroyImageView(m_Device->GetLogicalDevice(), m_DepthImageView, nullptr);
	vkDestroyImage(m_Device->GetLogicalDevice(), m_DepthImage, nullptr);
	vkFreeMemory(m_Device->GetLogicalDevice(), m_DepthImageMemory, nullptr);

	m_Texture.reset();
	
	imguiRenderer.reset();
	m_Descriptor.reset();
	m_Pipeline.reset();
	m_RenderPass.reset();
	m_Image.reset();
	m_SwapChain.reset();
	m_Device.reset();
	m_Window.reset();
}

VkFormat Renderer::FindDepthFormat() const
{
    return FindSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

VkFormat Renderer::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
{
    for (VkFormat format : candidates)
    {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(m_Device->GetPhysicalDevice(), format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
        {
            return format;
        }
    	else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
    	{
            return format;
        }
    }

    throw std::runtime_error("No suitable format found!");
}

void Renderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
                          VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const
{
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(m_Device->GetLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS)
    {
        throw std::runtime_error("Image couldn't created!");
    }
	LOG_INFO("Image created successfully!");

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_Device->GetLogicalDevice(), image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = m_Device->FindMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(m_Device->GetLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
    {
        throw std::runtime_error("Image memory couldn't allocated!");
    }
	LOG_INFO("Image memory allocated successfully!");

    vkBindImageMemory(m_Device->GetLogicalDevice(), image, imageMemory, 0);
}

VkImageView Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) const
{
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    if (vkCreateImageView(m_Device->GetLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Image view couldn't created!");
    }
	LOG_INFO("Image view created successfully!");

    return imageView;
}