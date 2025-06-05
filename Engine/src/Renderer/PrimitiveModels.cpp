#include "PrimitiveModels.h"
#include "Logger.h"

void PrimitiveModels::Initialize(Device* device)
{
    auto& data = GetPrimitiveData();
    if (data.initialized) {
        LOG_WARN("Primitive models already initialized");
        return;
    }
    
    LOG_INFO("Initializing primitive models with device...");
    
    try {
        // Create and buffer cube
        data.cubeModel = GenerateCubeMesh(1.0f);
        CreateBuffersForModel(data.cubeModel.get(), device);
        LOG_INFO("Cube model initialized - Meshes: {}, Vertices: {}, Indices: {}", 
                data.cubeModel->Meshes.size(),
                data.cubeModel->Meshes[0].Vertices.size(),
                data.cubeModel->Meshes[0].Indices.size());
        
        // Create and buffer sphere  
        data.sphereModel = GenerateSphereMesh(1.0f, 32, 16);
        CreateBuffersForModel(data.sphereModel.get(), device);
        LOG_INFO("Sphere model initialized");
        
        // Create and buffer plane
        data.planeModel = GeneratePlaneMesh(1.0f);
        CreateBuffersForModel(data.planeModel.get(), device);
        LOG_INFO("Plane model initialized");
        
        data.initialized = true;
        LOG_INFO("All primitive models initialized successfully");
    }
    catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize primitive models: {}", e.what());
        data.initialized = false;
    }
}

ECS::Entity PrimitiveModels::CreateCube(ECS::World* world, float size)
{
    if (!world) {
        LOG_ERROR("World is null!");
        return 0;
    }
    
    auto& data = GetPrimitiveData();
    if (!data.initialized || !data.cubeModel) {
        LOG_ERROR("Primitive models not initialized! Call PrimitiveModels::Initialize first!");
        return 0;
    }
    
    LOG_DEBUG("Creating primitive cube entity with size: {}", size);
    
    // Create entity
    ECS::Entity entity = world->CreateEntity();
    LOG_DEBUG("Created entity: {}", entity);
    
    // Add transform component
    TransformComponent transform;
    transform.scale = glm::vec3(size);
    world->AddComponent(entity, transform);
    LOG_DEBUG("Added TransformComponent with scale: {}", size);
    
    // Add model component with shared buffered model
    ModelComponent modelComp;
    modelComp.ModelPath = "primitive://cube";
    modelComp.ModelData = data.cubeModel;  // Use pre-buffered model
    modelComp.IsLoaded = true;
    modelComp.IsDirty = false;  // Buffers already exist!
    world->AddComponent(entity, modelComp);
    LOG_DEBUG("Added ModelComponent with {} meshes", data.cubeModel->Meshes.size());
    
    // Add renderable component
    world->AddComponent(entity, RenderableComponent(true));
    LOG_DEBUG("Added RenderableComponent");
    
    // Add material component
    world->AddComponent(entity, MaterialComponent(glm::vec3(0.7f, 0.7f, 0.7f)));
    LOG_DEBUG("Added MaterialComponent");
    
    // Add bounding component
    BoundingComponent bounds;
    bounds.Center = glm::vec3(0.0f);
    bounds.Extents = glm::vec3(size);
    bounds.UpdateBounds();
    world->AddComponent(entity, bounds);
    LOG_DEBUG("Added BoundingComponent");
    
    // Verify components
    if (!world->HasComponent<ModelComponent>(entity)) {
        LOG_ERROR("ModelComponent was not added properly!");
    }
    
    LOG_INFO("Cube entity {} created successfully with all components", entity);
    return entity;
}

ECS::Entity PrimitiveModels::CreateSphere(ECS::World* world, float radius, int segments, int rings)
{
    if (!world) {
        LOG_ERROR("World is null!");
        return 0;
    }
    
    auto& data = GetPrimitiveData();
    if (!data.initialized || !data.sphereModel) {
        LOG_ERROR("Primitive models not initialized! Call PrimitiveModels::Initialize first!");
        return 0;
    }
    
    LOG_DEBUG("Creating primitive sphere entity with radius: {}", radius);
    
    // Create entity
    ECS::Entity entity = world->CreateEntity();
    
    // Add transform component
    TransformComponent transform;
    transform.scale = glm::vec3(radius);
    world->AddComponent(entity, transform);
    
    // Add model component
    ModelComponent modelComp;
    modelComp.ModelPath = "primitive://sphere";
    modelComp.ModelData = data.sphereModel;
    modelComp.IsLoaded = true;
    modelComp.IsDirty = false;
    world->AddComponent(entity, modelComp);
    
    // Add other components
    world->AddComponent(entity, RenderableComponent(true));
    world->AddComponent(entity, MaterialComponent(glm::vec3(0.7f, 0.7f, 0.7f)));
    
    BoundingComponent bounds;
    bounds.Center = glm::vec3(0.0f);
    bounds.Extents = glm::vec3(radius);
    bounds.UpdateBounds();
    world->AddComponent(entity, bounds);
    
    LOG_INFO("Sphere entity {} created successfully", entity);
    return entity;
}

ECS::Entity PrimitiveModels::CreatePlane(ECS::World* world, float size)
{
    if (!world) {
        LOG_ERROR("World is null!");
        return 0;
    }
    
    auto& data = GetPrimitiveData();
    if (!data.initialized || !data.planeModel) {
        LOG_ERROR("Primitive models not initialized! Call PrimitiveModels::Initialize first!");
        return 0;
    }
    
    LOG_DEBUG("Creating primitive plane entity with size: {}", size);
    
    // Create entity
    ECS::Entity entity = world->CreateEntity();
    
    // Add transform component
    TransformComponent transform;
    transform.scale = glm::vec3(size, 1.0f, size);
    world->AddComponent(entity, transform);
    
    // Add model component
    ModelComponent modelComp;
    modelComp.ModelPath = "primitive://plane";
    modelComp.ModelData = data.planeModel;
    modelComp.IsLoaded = true;
    modelComp.IsDirty = false;
    world->AddComponent(entity, modelComp);
    
    // Add other components
    world->AddComponent(entity, RenderableComponent(true));
    world->AddComponent(entity, MaterialComponent(glm::vec3(0.8f, 0.8f, 0.8f)));
    
    BoundingComponent bounds;
    bounds.Center = glm::vec3(0.0f);
    bounds.Extents = glm::vec3(size, 0.1f, size);
    bounds.UpdateBounds();
    world->AddComponent(entity, bounds);
    
    LOG_INFO("Plane entity {} created successfully", entity);
    return entity;
}

std::shared_ptr<Model> PrimitiveModels::GenerateCubeMesh(float size)
{
    auto model = std::make_shared<Model>();
    
    // Define vertices for a cube
    std::vector<Vertex3D> vertices = {
        // Front face
        {{-size, -size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, -size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size,  size,  size}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size,  size,  size}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},

        // Back face
        {{-size, -size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, -size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size,  size, -size}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size,  size, -size}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},

        // Left face
        {{-size,  size,  size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size,  size, -size}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size, -size, -size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size, -size,  size}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        // Right face
        {{ size,  size,  size}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size,  size, -size}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, -size, -size}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, -size,  size}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        // Bottom face
        {{-size, -size, -size}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, -size, -size}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, -size,  size}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size, -size,  size}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},

        // Top face
        {{-size,  size, -size}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size,  size, -size}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size,  size,  size}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size,  size,  size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}}
    };

    // Define indices for cube faces
    std::vector<uint32_t> indices = {
        // Front face
        0,  1,  2,  2,  3,  0,   
        // Back face
        5,  4,  7,  7,  6,  5,
        // Left face
        8,  11, 10,  10, 9,  8,
        // Right face
        13, 12, 15,  15, 14, 13,
        // Bottom face
        16, 19, 18,  18, 17, 16,
        // Top face
        21, 20, 23,  23, 22, 21
    };

    model->Meshes.emplace_back(vertices, indices);
    return model;
}

std::shared_ptr<Model> PrimitiveModels::GenerateSphereMesh(float radius, int segments, int rings)
{
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
            
            // Color
            vertex.Color = glm::vec3(1.0f, 1.0f, 1.0f);
            
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
    return model;
}

std::shared_ptr<Model> PrimitiveModels::GeneratePlaneMesh(float size)
{
    auto model = std::make_shared<Model>();
    
    std::vector<Vertex3D> vertices = {
        {{-size, 0.0f, -size}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, 0.0f, -size}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{ size, 0.0f,  size}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}},
        {{-size, 0.0f,  size}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f, 1.0f}}
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2, 2, 3, 0
    };
    
    model->Meshes.emplace_back(vertices, indices);
    return model;
}

void PrimitiveModels::CreateBuffersForModel(Model* model, Device* device)
{
    if (!model || !device) {
        LOG_ERROR("Model or device is null!");
        return;
    }
    
    LOG_DEBUG("Creating buffers for {} meshes", model->Meshes.size());
    
    for (auto& mesh : model->Meshes) {
        if (mesh.Vertices.empty() || mesh.Indices.empty()) {
            LOG_ERROR("Mesh has empty vertices or indices!");
            continue;
        }
        
        try {
            CreateMeshBuffers(mesh, device);
            LOG_DEBUG("Buffers created - VB: {}, IB: {}", 
                    (void*)mesh.VertexBuffer, (void*)mesh.IndexBuffer);
        }
        catch (const std::exception& e) {
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