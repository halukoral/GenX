#pragma once
#include "ECS/ECS.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "Event/ApplicationEvent.h"
#include "Input/Input.h"
#include "Event/Event.h"
#include "Event/KeyEvent.h"
#include "Event/MouseEvent.h"

// Camera System - Updates view matrices
class CameraSystem : public ECS::System {
private:
    ECS::World* world;
    
public:
    struct CameraData {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;
        glm::vec3 position;
        glm::vec3 forward;
        glm::vec3 right;
        glm::vec3 up;
    };

	CameraSystem() = default;

	void setWorld(ECS::World* w) { world = w; }
    
    void update(float dt) override {
        for (auto entity : entities) {
            auto& transform = world->getComponent<TransformComponent>(entity);
            auto& camera = world->getComponent<CameraComponent>(entity);
            
            if (!camera.isActive) continue;
            
            // Update aspect ratio if needed (handled by window resize event)
            
            // Calculate view matrix
            glm::vec3 forward = transform.getForward();
            glm::vec3 up = transform.getUp();
            glm::mat4 view = glm::lookAt(
                transform.position,
                transform.position + forward,
                up
            );
            
            // Store camera data for renderer
            currentCameraData.view = view;
            currentCameraData.projection = camera.getProjectionMatrix();
            currentCameraData.viewProjection = camera.getProjectionMatrix() * view;
            currentCameraData.position = transform.position;
            currentCameraData.forward = forward;
            currentCameraData.right = transform.getRight();
            currentCameraData.up = up;
            
            // For Vulkan coordinate system (flip Y in projection)
            currentCameraData.projection[1][1] *= -1;
        }
    }
    
    const CameraData& getCurrentCameraData() const { return currentCameraData; }
    
    void setAspectRatio(float aspect) {
        for (auto entity : entities) {
            auto& camera = world->getComponent<CameraComponent>(entity);
            camera.aspectRatio = aspect;
        }
    }
    
private:
    CameraData currentCameraData;
};

// Camera Controller System - Handles input for camera movement
class CameraControllerSystem : public ECS::System {
private:
    ECS::World* world;
    
public:
	CameraControllerSystem() = default;

	void setWorld(ECS::World* w) { world = w; }
	
    void update(float dt) override
	{
        for (auto entity : entities)
        {
            auto& transform = world->getComponent<TransformComponent>(entity);
            auto& controller = world->getComponent<CameraControllerComponent>(entity);
            
            if (!controller.isControllable) continue;
            
            // Get input
            glm::vec3 movement(0.0f);
            float speed = controller.moveSpeed;
            
            // Sprint
            if (Input::IsKeyDown(KeyCode::LeftShift))
            {
                speed *= controller.sprintMultiplier;
            }
            
            // Movement
            if (Input::IsKeyDown(KeyCode::W)) movement += transform.getForward();
            if (Input::IsKeyDown(KeyCode::S)) movement -= transform.getForward();
            if (Input::IsKeyDown(KeyCode::A)) movement -= transform.getRight();
            if (Input::IsKeyDown(KeyCode::D)) movement += transform.getRight();
            if (Input::IsKeyDown(KeyCode::Q)) movement -= glm::vec3(0, 1, 0);
            if (Input::IsKeyDown(KeyCode::E)) movement += glm::vec3(0, 1, 0);
            
            // Normalize and apply movement
            if (glm::length(movement) > 0.0f)
            {
                movement = glm::normalize(movement) * speed * dt;
                transform.position += movement;
            }
            
            // Mouse look
            glm::vec2 mousePos = Input::GetMousePosition();
            
            if (controller.firstMouse)
            {
                controller.lastMousePos = mousePos;
                controller.firstMouse = false;
            }
            
            glm::vec2 delta = mousePos - controller.lastMousePos;
            controller.lastMousePos = mousePos;
            
            // Only rotate if right mouse button is held
            if (Input::IsMouseButtonDown(MouseButton::Right))
            {
                delta *= controller.mouseSensitivity;
                
                // Update euler angles
                glm::vec3 euler = transform.eulerAngles;
                euler.y -= delta.x; // Yaw
                euler.x -= delta.y; // Pitch
                
                // Clamp pitch
                euler.x = glm::clamp(euler.x, -controller.maxPitch, controller.maxPitch);
                
                transform.setEulerAngles(euler);
            }
        }
    }
    
    void onEvent(Event& e)
	{
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e) {
            return onMouseMoved(e);
        });
    }
    
private:
    bool onMouseMoved(MouseMovedEvent& e) {
        // Additional mouse handling if needed
        return false;
    }
};

class CameraManager {
private:
    ECS::World* world;
    CameraSystem* cameraSystem;
    CameraControllerSystem* controllerSystem;
    ECS::Entity activeCamera = 0;
    
public:
    CameraManager(ECS::World* ecsWorld) : world(ecsWorld) {
        // Register systems
        cameraSystem = world->registerSystem<CameraSystem>().get();
    	cameraSystem->setWorld(world);
        controllerSystem = world->registerSystem<CameraControllerSystem>().get();
    	controllerSystem->setWorld(world);
    	
        // Set system signatures
        ECS::Signature cameraSignature;
        cameraSignature.set(ECS::ComponentTypeCounter::getTypeID<TransformComponent>());
        cameraSignature.set(ECS::ComponentTypeCounter::getTypeID<CameraComponent>());
        world->setSystemSignature<CameraSystem>(cameraSignature);
        
        ECS::Signature controllerSignature;
        controllerSignature.set(ECS::ComponentTypeCounter::getTypeID<TransformComponent>());
        controllerSignature.set(ECS::ComponentTypeCounter::getTypeID<CameraControllerComponent>());
        world->setSystemSignature<CameraControllerSystem>(controllerSignature);
    }
    
    ECS::Entity createCamera(const glm::vec3& position, float fov = 45.0f, float aspect = 16.0f/9.0f) {
        ECS::Entity camera = world->createEntity();
        
        // Add transform
        world->addComponent(camera, TransformComponent(position));
        
        // Add camera component
        world->addComponent(camera, CameraComponent(fov, aspect));
        
        // Set as active if first camera
        if (activeCamera == 0) {
            activeCamera = camera;
        }
        
        return camera;
    }
    
    ECS::Entity createFPSCamera(const glm::vec3& position, float fov = 60.0f, float aspect = 16.0f/9.0f) {
        ECS::Entity camera = createCamera(position, fov, aspect);
        
        // Add controller component
        world->addComponent(camera, CameraControllerComponent());
        
        return camera;
    }
    
    void setActiveCamera(ECS::Entity camera) {
        if (world->hasComponent<CameraComponent>(camera)) {
            // Deactivate previous camera
            if (activeCamera != 0 && world->hasComponent<CameraComponent>(activeCamera)) {
                world->getComponent<CameraComponent>(activeCamera).isActive = false;
            }
            
            // Activate new camera
            activeCamera = camera;
            world->getComponent<CameraComponent>(camera).isActive = true;
        }
    }
    
    ECS::Entity getActiveCamera() const { return activeCamera; }
    
    const CameraSystem::CameraData& getCameraData() const {
        return cameraSystem->getCurrentCameraData();
    }
    
    void update(float dt) {
        world->update(dt);
    }
    
    void onEvent(Event& e) {
        controllerSystem->onEvent(e);
        
        // Handle window resize
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) {
            float aspect = static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight());
            cameraSystem->setAspectRatio(aspect);
            return false;
        });
    }
    
    // Utility functions
    void setCameraPosition(ECS::Entity camera, const glm::vec3& position) {
        if (world->hasComponent<TransformComponent>(camera)) {
            world->getComponent<TransformComponent>(camera).position = position;
        }
    }
    
    void setCameraRotation(ECS::Entity camera, const glm::vec3& eulerAngles) {
        if (world->hasComponent<TransformComponent>(camera)) {
            world->getComponent<TransformComponent>(camera).setEulerAngles(eulerAngles);
        }
    }
    
    void enableCameraControl(ECS::Entity camera, bool enable) {
        if (world->hasComponent<CameraControllerComponent>(camera)) {
            world->getComponent<CameraControllerComponent>(camera).isControllable = enable;
        }
    }
};