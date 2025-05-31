#pragma once
#include "ECS/ECS.h"
#include "Renderer/Model.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

// Material Component - Material properties for rendering
struct MaterialComponent : public ECS::Component
{
    glm::vec3 albedo = glm::vec3(1.0f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f;
    
    // Texture paths
    std::string diffuseTexture = "";
    std::string normalTexture = "";
    std::string metallicTexture = "";
    std::string roughnessTexture = "";
    std::string aoTexture = "";
    
    MaterialComponent() = default;
    MaterialComponent(const glm::vec3& color, float metal = 0.0f, float rough = 0.5f)
        : albedo(color), metallic(metal), roughness(rough) {}
};

// Model Component - References to 3D model data
struct ModelComponent : public ECS::Component
{
    std::string modelPath = "";
    std::shared_ptr<Model> modelData = nullptr;
    bool isLoaded = false;
    bool isDirty = false; // Needs buffer recreation
    
    // Model-specific settings
    bool castShadows = true;
    bool receiveShadows = true;
    
    ModelComponent() = default;
    ModelComponent(const std::string& path) : modelPath(path) {}
    
    // Helper method to check if model is ready for rendering
    bool IsReadyForRender() const {
        return isLoaded && modelData != nullptr && !modelData->Meshes.empty();
    }
};

// Renderable Component - Controls rendering behavior
struct RenderableComponent : public ECS::Component
{
    bool isVisible = true;
    bool frustumCulling = true;
    float lodDistance = 100.0f;
    int renderLayer = 0; // For render order
    
    // Render flags
    bool wireframe = false;
    bool backfaceCulling = true;
    
    RenderableComponent() = default;
    RenderableComponent(bool visible, int layer = 0) 
        : isVisible(visible), renderLayer(layer) {}
};

// Bounding Component - For frustum culling and collision
struct BoundingComponent : public ECS::Component
{
    glm::vec3 center = glm::vec3(0.0f);
    glm::vec3 extents = glm::vec3(1.0f);
    float radius = 1.0f;
    
    // Bounding box corners (calculated from center and extents)
    glm::vec3 min = glm::vec3(-0.5f);
    glm::vec3 max = glm::vec3(0.5f);
    
    BoundingComponent() = default;
    BoundingComponent(const glm::vec3& center, const glm::vec3& extents)
        : center(center), extents(extents) {
        UpdateBounds();
    }
    
    void UpdateBounds() {
        min = center - extents * 0.5f;
        max = center + extents * 0.5f;
        radius = glm::length(extents) * 0.5f;
    }
    
    // Check if point is inside bounding box
    bool Contains(const glm::vec3& point) const {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y &&
               point.z >= min.z && point.z <= max.z;
    }
    
    // Check if bounding box intersects with another
    bool Intersects(const BoundingComponent& other) const {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y &&
               min.z <= other.max.z && max.z >= other.min.z;
    }
};