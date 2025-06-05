#include "PrimitiveModels.h"

std::shared_ptr<Model> PrimitiveModels::CreateCube(float size, Device* device)
{
	LOG_DEBUG("Creating primitive cube with size: {}", size);
        
	auto model = std::make_shared<Model>();
        
	// Define vertices for a cube
	std::vector<Vertex3D> vertices = {
		// Front face
		{{-size, -size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
		{{ size, -size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
		{{ size,  size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
		{{-size,  size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}},

		// Back face
		{{-size, -size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}},
		{{ size, -size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}},
		{{ size,  size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {0.5f, 0.5f, 0.5f}},
		{{-size,  size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {0.8f, 0.2f, 0.6f}},

		// Left face
		{{-size,  size,  size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.5f, 0.0f}},
		{{-size,  size, -size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.5f, 1.0f, 0.0f}},
		{{-size, -size, -size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.5f, 1.0f}},
		{{-size, -size,  size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.5f}},

		// Right face
		{{ size,  size,  size}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {0.8f, 0.3f, 0.1f}},
		{{ size,  size, -size}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 0.8f, 0.3f}},
		{{ size, -size, -size}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {0.6f, 0.2f, 0.8f}},
		{{ size, -size,  size}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {0.9f, 0.7f, 0.2f}},

		// Bottom face
		{{-size, -size, -size}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {0.4f, 0.6f, 0.9f}},
		{{ size, -size, -size}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {0.7f, 0.4f, 0.6f}},
		{{ size, -size,  size}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {0.3f, 0.9f, 0.5f}},
		{{-size, -size,  size}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.5f, 0.3f}},

		// Top face
		{{-size,  size, -size}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.6f, 0.8f, 0.2f}},
		{{ size,  size, -size}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.2f, 0.6f, 0.8f}},
		{{ size,  size,  size}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.9f, 0.3f, 0.7f}},
		{{-size,  size,  size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.5f, 0.9f, 0.4f}}
	};

	// Define indices for cube faces
	std::vector<uint32_t> indices = {
		0,  1,  2,  2,  3,  0,   // front
		4,  5,  6,  6,  7,  4,   // back
		8,  9,  10, 10, 11, 8,   // left
		12, 13, 14, 14, 15, 12,  // right
		16, 17, 18, 18, 19, 16,  // bottom
		20, 21, 22, 22, 23, 20   // top
	};

	model->Meshes.emplace_back(vertices, indices);
        
	if (device)
	{
		LOG_DEBUG("Creating buffers for cube primitive...");
		CreateBuffersForModel(model.get(), device);
	}
        
	// Validation
	if (model->Meshes.empty())
	{
		LOG_ERROR("Cube model has no meshes!");
		return nullptr;
	}
        
	const auto& mesh = model->Meshes[0];
	if (mesh.Vertices.empty() || mesh.Indices.empty())
	{
		LOG_ERROR("Cube mesh is empty! Vertices: {}, Indices: {}", 
				mesh.Vertices.size(), mesh.Indices.size());
		return nullptr;
	}
        
	LOG_DEBUG("Cube created successfully - Vertices: {}, Indices: {}, VB: {}, IB: {}", 
			mesh.Vertices.size(), mesh.Indices.size(), 
			(void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
        
	return model;
}

std::shared_ptr<Model> PrimitiveModels::CreateSphere(float radius, int segments, int rings, Device* device)
{
	LOG_DEBUG("Creating primitive sphere with radius: {}", radius);

	auto model = std::make_shared<Model>();

	std::vector<Vertex3D> vertices;
	std::vector<uint32_t> indices;

	// Generate sphere vertices
	for (int ring = 0; ring <= rings; ++ring)
	{
	    float phi = static_cast<float>(ring) * M_PI / static_cast<float>(rings);
	    
	    for (int segment = 0; segment <= segments; ++segment)
	    {
	        float theta = static_cast<float>(segment) * 2.0f * M_PI / static_cast<float>(segments);
	        
	        Vertex3D vertex;
	        
	        // Position
	        vertex.Pos.x = radius * sin(phi) * cos(theta);
	        vertex.Pos.y = radius * cos(phi);
	        vertex.Pos.z = radius * sin(phi) * sin(theta);
	        
	        // Normal (normalized position for sphere)
	        vertex.Normal = glm::normalize(vertex.Pos);
	        
	        // Texture coordinates
	        vertex.TexCoord.x = static_cast<float>(segment) / static_cast<float>(segments);
	        vertex.TexCoord.y = static_cast<float>(ring) / static_cast<float>(rings);
	        
	        // Color (can be customized)
	        vertex.Color = glm::vec3(0.7f, 0.7f, 0.7f);
	        
	        vertices.push_back(vertex);
	    }
	}

	// Generate sphere indices
	for (int ring = 0; ring < rings; ++ring)
	{
	    for (int segment = 0; segment < segments; ++segment)
	    {
	        int current = ring * (segments + 1) + segment;
	        int next = current + segments + 1;
	        
	        // Two triangles per quad
	        indices.push_back(current);
	        indices.push_back(next);
	        indices.push_back(current + 1);
	        
	        indices.push_back(current + 1);
	        indices.push_back(next);
	        indices.push_back(next + 1);
	    }
	}

	model->Meshes.emplace_back(vertices, indices);

	if (device)
	{
	    LOG_DEBUG("Creating buffers for sphere primitive...");
	    CreateBuffersForModel(model.get(), device);
	}

	// Validation
	const auto& mesh = model->Meshes[0];
	LOG_DEBUG("Sphere created successfully - Vertices: {}, Indices: {}, VB: {}, IB: {}", 
	         mesh.Vertices.size(), mesh.Indices.size(), 
	         (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);

	return model;       
}

std::shared_ptr<Model> PrimitiveModels::CreatePlane(float size, Device* device)
{
	auto model = std::make_shared<Model>();
        
	std::vector<Vertex3D> vertices = {
		{{-size, 0.0f, -size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
		{{ size, 0.0f, -size}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {0.8f, 0.8f, 0.8f}},
		{{ size, 0.0f,  size}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {0.8f, 0.8f, 0.8f}},
		{{-size, 0.0f,  size}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {0.8f, 0.8f, 0.8f}}
	};
        
	std::vector<uint32_t> indices =
	{
		0, 1, 2, 2, 3, 0
	};
        
	model->Meshes.emplace_back(vertices, indices);
        
	if (device)
	{
		LOG_DEBUG("Creating buffers for plane primitive...");
		CreateBuffersForModel(model.get(), device);
	}
        
	return model;
}

void PrimitiveModels::CreateBuffersForModel(Model* model, Device* device)
{
	if (!model || !device)
	{
		LOG_ERROR("Model or device is null!");
		return;
	}
        
	LOG_DEBUG("Creating buffers for {} meshes", model->Meshes.size());
        
	for (auto& mesh : model->Meshes)
	{
		if (mesh.Vertices.empty() || mesh.Indices.empty())
		{
			LOG_ERROR("Mesh has empty vertices or indices!");
			continue;
		}
            
		try
		{
			CreateMeshBuffers(mesh, device);
			LOG_DEBUG("Buffers created - VB: {}, IB: {}", 
					(void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
		}
		catch (const std::exception& e)
		{
			LOG_ERROR("Failed to create mesh buffers: {}", e.what());
		}
	}
}

void PrimitiveModels::CreateMeshBuffers(Mesh& mesh, Device* device)
{
	// Vertex buffer
	const VkDeviceSize vertexBufferSize = sizeof(mesh.Vertices[0]) * mesh.Vertices.size();
	CreateBufferWithData(vertexBufferSize, mesh.Vertices.data(),
						VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
						mesh.VertexBuffer, mesh.VertexBufferMemory, device);
        
	// Index buffer
	const VkDeviceSize indexBufferSize = sizeof(mesh.Indices[0]) * mesh.Indices.size();
	CreateBufferWithData(indexBufferSize, mesh.Indices.data(),
						VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
						mesh.IndexBuffer, mesh.IndexBufferMemory, device);
}

void PrimitiveModels::CreateBufferWithData(VkDeviceSize size, const void* data, VkBufferUsageFlags usage,
	VkBuffer& buffer, VkDeviceMemory& bufferMemory, Device* device)
{
	// Create staging buffer
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				stagingBuffer, stagingBufferMemory, device);
        
	// Copy data to staging buffer
	void* mapped;
	vkMapMemory(device->GetLogicalDevice(), stagingBufferMemory, 0, size, 0, &mapped);
	memcpy(mapped, data, static_cast<size_t>(size));
	vkUnmapMemory(device->GetLogicalDevice(), stagingBufferMemory);
        
	// Create device buffer
	CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, buffer, bufferMemory, device);
        
	// Copy from staging to device buffer
	CopyBuffer(stagingBuffer, buffer, size, device);
        
	// Cleanup staging buffer
	vkDestroyBuffer(device->GetLogicalDevice(), stagingBuffer, nullptr);
	vkFreeMemory(device->GetLogicalDevice(), stagingBufferMemory, nullptr);
}

void PrimitiveModels::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
	VkBuffer& buffer, VkDeviceMemory& bufferMemory, Device* device)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
	if (vkCreateBuffer(device->GetLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create buffer!");
	}
        
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device->GetLogicalDevice(), buffer, &memRequirements);
        
	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device->FindMemoryType(memRequirements.memoryTypeBits, properties);
        
	if (vkAllocateMemory(device->GetLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate buffer memory!");
	}
        
	vkBindBufferMemory(device->GetLogicalDevice(), buffer, bufferMemory, 0);
}

void PrimitiveModels::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, Device* device)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device->GetCommandPool();
	allocInfo.commandBufferCount = 1;
        
	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device->GetLogicalDevice(), &allocInfo, &commandBuffer);
        
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
        
	vkQueueSubmit(device->GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(device->GetGraphicsQueue());
        
	vkFreeCommandBuffers(device->GetLogicalDevice(), device->GetCommandPool(), 1, &commandBuffer);
}
