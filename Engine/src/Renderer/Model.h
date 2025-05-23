#pragma once

#include "pch.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Vertex3D
{
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color;

    static VkVertexInputBindingDescription GetBindingDescription();

    static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions();

    bool operator==(const Vertex3D& other) const
    {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord && color == other.color;
    }
};

class Mesh
{
public:
    std::vector<Vertex3D> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;

    Mesh(std::vector<Vertex3D> verts, std::vector<uint32_t> inds) : vertices(verts), indices(inds) {}

    void Cleanup(VkDevice device) const;
};

class Model
{
public:
    std::vector<Mesh> meshes;
    std::string directory;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    Model() = default;

    Model(const std::string& path);

    void cleanup(VkDevice device);

    glm::mat4 getModelMatrix();

    static Model createCube();
    static Model loadFromFile(const std::string& path);

private:
    
    void loadModel(const std::string& path);
    void calculateNormals(std::vector<Vertex3D>& vertices, const std::vector<uint32_t>& indices);
};