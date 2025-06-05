#pragma once
#include "ECS/ECS.h"
#include "Renderer/Model.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>

// Material Component - Material properties for rendering
struct MaterialComponent : public ECS::Component
{
    glm::vec3 Albedo = glm::vec3(1.0f);
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    float Ao = 1.0f;
    
    // Texture paths
    std::string DiffuseTexture;
    std::string NormalTexture;
    std::string MetallicTexture;
    std::string RoughnessTexture;
    std::string AoTexture;
    
    MaterialComponent() = default;
    MaterialComponent(const glm::vec3& color, float metal = 0.0f, float rough = 0.5f)
        : Albedo(color), Metallic(metal), Roughness(rough) {}
};

// Model Component - References to 3D model data
struct ModelComponent : public ECS::Component
{
    std::string ModelPath;
    std::shared_ptr<Model> ModelData = nullptr;
    bool IsLoaded = false;
    bool IsDirty = false; // Needs buffer recreation
    
    // Model-specific settings
    bool CastShadows = true;
    bool ReceiveShadows = true;
    
    ModelComponent() = default;
    ModelComponent(const std::string& path) : ModelPath(path) {}
    
    // Helper method to check if model is ready for rendering
    bool IsReadyForRender() const
	{
        return IsLoaded && ModelData != nullptr && !ModelData->Meshes.empty();
    }
};

// Renderable Component - Controls rendering behavior
struct RenderableComponent : public ECS::Component
{
    bool IsVisible = true;
    bool FrustumCulling = true;
    float LodDistance = 100.0f;
    int RenderLayer = 0; // For render order
    
    // Render flags
    bool Wireframe = false;
    bool BackfaceCulling = true;
    
    RenderableComponent() = default;
    RenderableComponent(const bool visible, const int layer = 0) 
        : IsVisible(visible), RenderLayer(layer) {}
};

// Bounding Component - For frustum culling and collision
struct BoundingComponent : public ECS::Component
{
    glm::vec3 Center = glm::vec3(0.0f);
    glm::vec3 Extents = glm::vec3(1.0f);
    float Radius = 1.0f;
    
    // Bounding box corners (calculated from center and extents)
    glm::vec3 Min = glm::vec3(-0.5f);
    glm::vec3 Max = glm::vec3(0.5f);
    
    BoundingComponent() = default;
    BoundingComponent(const glm::vec3& center, const glm::vec3& extents)
        : Center(center), Extents(extents)
	{
        UpdateBounds();
    }
    
    void UpdateBounds()
	{
        Min = Center - Extents * 0.5f;
        Max = Center + Extents * 0.5f;
        Radius = glm::length(Extents) * 0.5f;
    }
    
    // Check if point is inside bounding box
    bool Contains(const glm::vec3& point) const
	{
        return point.x >= Min.x && point.x <= Max.x &&
               point.y >= Min.y && point.y <= Max.y &&
               point.z >= Min.z && point.z <= Max.z;
    }
    
    // Check if bounding box intersects with another
    bool Intersects(const BoundingComponent& other) const
	{
        return Min.x <= other.Max.x && Max.x >= other.Min.x &&
               Min.y <= other.Max.y && Max.y >= other.Min.y &&
               Min.z <= other.Max.z && Max.z >= other.Min.z;
    }
};