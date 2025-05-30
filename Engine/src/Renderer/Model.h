#pragma once

#include "pch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Vertex3D
{
    glm::vec3 Pos;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec3 Color;

    static VkVertexInputBindingDescription GetBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions();

    bool operator==(const Vertex3D& other) const
    {
        return Pos == other.Pos && Normal == other.Normal && TexCoord == other.TexCoord && Color == other.Color;
    }
};

// Uniform Buffer Object
struct UniformBufferObject
{
    glm::mat4 Model;
    glm::mat4 View;
    glm::mat4 Proj;
};

class Mesh
{
public:
    std::vector<Vertex3D> Vertices;
    std::vector<uint32_t> Indices;
    
    VkBuffer VertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory VertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer IndexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory IndexBufferMemory = VK_NULL_HANDLE;

    Mesh(const std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices) : Vertices(vertices), Indices(indices) {}

    void Cleanup(VkDevice device) const;
};

class Model
{
public:
    std::vector<Mesh> Meshes;
    std::string Directory;
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f);
    glm::vec3 Scale = glm::vec3(1.0f);

    Model() = default;
    Model(const std::string& path);

    void Cleanup(VkDevice device) const;

    glm::mat4 GetModelMatrix() const;

    static Model CreateCube();
    static Model LoadFromFile(const std::string& path);

private:
    void LoadModel(const std::string& path);
    void CalculateNormals(std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices);
};