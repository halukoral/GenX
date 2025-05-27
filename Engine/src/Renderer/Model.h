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
class Camera {
public:
    // Kamera hareket yönleri
    enum Camera_Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    // Kamera ayarları
    static constexpr float DEFAULT_YAW = -90.0f;
    static constexpr float DEFAULT_PITCH = 0.0f;
    static constexpr float DEFAULT_SPEED = 2.5f;
    static constexpr float DEFAULT_SENSITIVITY = 0.1f;
    static constexpr float DEFAULT_ZOOM = 45.0f;

private:
    // Kamera özellikleri
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;
    
    // Euler açıları
    float yaw;
    float pitch;
    
    // Kamera seçenekleri
    float movementSpeed;
    float mouseSensitivity;
    float zoom;

public:
    // Constructor
    Camera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, 3.0f), 
           glm::vec3 worldUpVec = glm::vec3(0.0f, 1.0f, 0.0f),
           float yawAngle = DEFAULT_YAW, 
           float pitchAngle = DEFAULT_PITCH) 
        : position(pos), worldUp(worldUpVec), yaw(yawAngle), pitch(pitchAngle),
          movementSpeed(DEFAULT_SPEED), mouseSensitivity(DEFAULT_SENSITIVITY), zoom(DEFAULT_ZOOM) 
    {
        updateCameraVectors();
    }

    // Matrices
    glm::mat4 getViewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    glm::mat4 getProjectionMatrix(float aspect) {
        return glm::perspective(glm::radians(zoom), aspect, 0.1f, 100.0f);
    }

    // Getters
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    float getZoom() const { return zoom; }
    float getYaw() const { return yaw; }
    float getPitch() const { return pitch; }

    // Kamera hareket işlemleri
    void processKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = movementSpeed * deltaTime;
        
        switch(direction) {
            case FORWARD:
                position += front * velocity;
                break;
            case BACKWARD:
                position -= front * velocity;
                break;
            case LEFT:
                position -= right * velocity;
                break;
            case RIGHT:
                position += right * velocity;
                break;
            case UP:
                position += worldUp * velocity;
                break;
            case DOWN:
                position -= worldUp * velocity;
                break;
        }
    }

    // Mouse hareket işlemi
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) {
        xoffset *= mouseSensitivity;
        yoffset *= mouseSensitivity;

        yaw += xoffset;
        pitch += yoffset;

        // Pitch sınırlarını belirle
        if (constrainPitch) {
            if (pitch > 89.0f)
                pitch = 89.0f;
            if (pitch < -89.0f)
                pitch = -89.0f;
        }

        // Kamera vektörlerini güncelle
        updateCameraVectors();
    }

    // Mouse scroll işlemi (zoom)
    void processMouseScroll(float yoffset) {
        zoom -= yoffset;
        if (zoom < 1.0f)
            zoom = 1.0f;
        if (zoom > 45.0f)
            zoom = 45.0f;
    }

    // Kamera ayarları
    void setMovementSpeed(float speed) { movementSpeed = speed; }
    void setMouseSensitivity(float sensitivity) { mouseSensitivity = sensitivity; }
    void setPosition(const glm::vec3& pos) { 
        position = pos; 
        updateCameraVectors();
    }

    // Reset kamera
    void reset() {
        position = glm::vec3(0.0f, 0.0f, 3.0f);
        yaw = DEFAULT_YAW;
        pitch = DEFAULT_PITCH;
        zoom = DEFAULT_ZOOM;
        updateCameraVectors();
    }

    // Debug bilgisi
    void printDebugInfo() const {
        std::cout << "=== Camera Debug Info ===" << std::endl;
        std::cout << "Position: (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
        std::cout << "Front: (" << front.x << ", " << front.y << ", " << front.z << ")" << std::endl;
        std::cout << "Up: (" << up.x << ", " << up.y << ", " << up.z << ")" << std::endl;
        std::cout << "Yaw: " << yaw << ", Pitch: " << pitch << ", Zoom: " << zoom << std::endl;
        std::cout << "Speed: " << movementSpeed << ", Sensitivity: " << mouseSensitivity << std::endl;
    }

private:
    // Kamera vektörlerini güncelle
    void updateCameraVectors() {
        // Front vektörünü hesapla
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);
        
        // Right ve Up vektörlerini yeniden hesapla
        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));
    }
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