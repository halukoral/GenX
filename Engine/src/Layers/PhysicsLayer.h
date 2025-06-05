#pragma once

#include "Layer.h"
#include "ECS/ECS.h"
#include "ECS/Systems/PhysicsManager.h"
#include "ECS/Components/ModelComponent.h"
#include "Layers/ModelLayer.h"
#include "Renderer/PrimitiveModels.h"
#include "Renderer/Device.h"  // EKLENEN - Device için gerekli
#include <memory>
#include <vector>

class PhysicsLayer : public Layer
{
private:
    std::unique_ptr<PhysicsManager> physicsManager;
    ModelLayer* modelLayer = nullptr; // Reference to model layer
    ECS::World* sharedWorld = nullptr; // ModelLayer'ın world'ünü kullanacağız
    
    // Demo entities for testing
    std::vector<ECS::Entity> demoEntities;
    bool demoMode = false;
    
    // Primitive models for physics visualization
    std::shared_ptr<Model> cubeModel = nullptr;
    std::shared_ptr<Model> sphereModel = nullptr;
    
public:
    ~PhysicsLayer() override = default;
    
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnEvent(Event& event) override;
    
    // Public interface for other systems
    PhysicsManager* GetPhysicsManager() const { return physicsManager.get(); }
    
    // Model layer integration
    void SetModelLayer(ModelLayer* layer) { modelLayer = layer; }
    
    // Demo controls
    void EnableDemo(bool enable) { demoMode = enable; }
    void CreatePhysicsDemo();
    void ClearDemo();
    
    // Utility functions
    ECS::Entity CreatePhysicsBox(const glm::vec3& position, 
                                const glm::vec3& size = glm::vec3(0.5f),
                                float mass = 1.0f) const;
    
    ECS::Entity CreatePhysicsSphere(const glm::vec3& position,
                                   float radius = 0.5f,
                                   float mass = 1.0f) const;
    
    void AddExplosion(const glm::vec3& position, float force = 10.0f, float radius = 5.0f);
    
private:
    // Helper function to add visual representation to physics entities
    void AddVisualToPhysicsEntity(ECS::Entity entity, const std::string& type);
    void AddRandomPhysicsObjects();
    void HandleDemoInput();
};