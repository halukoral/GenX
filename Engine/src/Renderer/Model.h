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

// Uniform Buffer Object
struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

// Camera Class - 3D kamera kontrolü
class Camera {
private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    
    float yaw;
    float pitch;
    float zoom;

public:
    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 worldUpVec = glm::vec3(0.0f, 1.0f, 0.0f),
           float yawAngle = -90.0f, float pitchAngle = 0.0f) 
        : position(pos), worldUp(worldUpVec), yaw(yawAngle), pitch(pitchAngle), zoom(45.0f) {
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspect) {
        return glm::perspective(glm::radians(zoom), aspect, 0.1f, 100.0f);
    }

    glm::vec3 getPosition() const { return position; }
    float getZoom() const { return zoom; }

private:
    void updateCameraVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
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