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

// Camera Class
class Camera
{
public:
    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 worldUpVec = glm::vec3(0.0f, 1.0f, 0.0f),
           float yawAngle = -90.0f, float pitchAngle = 0.0f) 
        : position(pos), worldUp(worldUpVec), yaw(yawAngle), pitch(pitchAngle), zoom(45.0f) {
        UpdateCameraVectors();
    }

    glm::mat4 GetViewMatrix() const
	{
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 GetProjectionMatrix(float aspect) const
	{
        return glm::perspective(glm::radians(zoom), aspect, 0.1f, 100.0f);
    }

    glm::vec3 GetPosition() const { return position; }
    float GetZoom() const { return zoom; }

private:
    void UpdateCameraVectors()
	{
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }

private:
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;
    
	float yaw;
	float pitch;
	float zoom;
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