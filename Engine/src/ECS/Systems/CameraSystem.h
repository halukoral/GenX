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
    struct CameraData
	{
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;
        glm::vec3 position;
        glm::vec3 forward;
        glm::vec3 right;
        glm::vec3 up;
    };

	CameraSystem() = default;

	void SetWorld(ECS::World* w) { world = w; }
    
    void Update(float dt) override
	{
        for (auto entity : Entities)
        {
            auto& transform = world->GetComponent<TransformComponent>(entity);
            auto& camera = world->GetComponent<CameraComponent>(entity);
            
            if (!camera.isActive) continue;
            
            // Update aspect ratio if needed (handled by window resize event)
            
            // Calculate view matrix
            glm::vec3 forward = transform.GetForward();
            glm::vec3 up = transform.GetUp();
            glm::mat4 view = glm::lookAt(
                transform.position,
                transform.position + forward,
                up
            );
            
            // Store camera data for renderer
            currentCameraData.view = view;
            currentCameraData.projection = camera.GetProjectionMatrix();
            currentCameraData.viewProjection = camera.GetProjectionMatrix() * view;
            currentCameraData.position = transform.position;
            currentCameraData.forward = forward;
            currentCameraData.right = transform.GetRight();
            currentCameraData.up = up;
            
            // For Vulkan coordinate system (flip Y in projection)
            currentCameraData.projection[1][1] *= -1;
        }
    }
    
    const CameraData& GetCurrentCameraData() const { return currentCameraData; }
    
    void SetAspectRatio(float aspect) const
	{
        for (auto entity : Entities)
        {
            auto& camera = world->GetComponent<CameraComponent>(entity);
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

	void SetWorld(ECS::World* w) { world = w; }
	
    void Update(float dt) override
	{
        for (auto entity : Entities)
        {
            auto& transform = world->GetComponent<TransformComponent>(entity);
            auto& controller = world->GetComponent<CameraControllerComponent>(entity);
            
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
            if (Input::IsKeyDown(KeyCode::W)) movement += transform.GetForward();
            if (Input::IsKeyDown(KeyCode::S)) movement -= transform.GetForward();
            if (Input::IsKeyDown(KeyCode::A)) movement -= transform.GetRight();
            if (Input::IsKeyDown(KeyCode::D)) movement += transform.GetRight();
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
                
                transform.SetEulerAngles(euler);
            }
        }
    }
    
    void OnEvent(Event& e) const
	{
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& e)
        {
            return OnMouseMoved(e);
        });
    }
    
private:
	static bool OnMouseMoved(MouseMovedEvent& e)
	{
        // Additional mouse handling if needed
        return false;
    }
};

class CameraManager
{
private:
    ECS::World* world;
    CameraSystem* cameraSystem;
    CameraControllerSystem* controllerSystem;
    ECS::Entity activeCamera = 0;
    
public:
    CameraManager(ECS::World* ecsWorld) : world(ecsWorld)
	{
        // Register systems
        cameraSystem = world->RegisterSystem<CameraSystem>().get();
    	cameraSystem->SetWorld(world);
        controllerSystem = world->RegisterSystem<CameraControllerSystem>().get();
    	controllerSystem->SetWorld(world);
    	
        // Set system signatures
        ECS::Signature cameraSignature;
        cameraSignature.set(ECS::ComponentTypeCounter::GetTypeId<TransformComponent>());
        cameraSignature.set(ECS::ComponentTypeCounter::GetTypeId<CameraComponent>());
        world->SetSystemSignature<CameraSystem>(cameraSignature);
        
        ECS::Signature controllerSignature;
        controllerSignature.set(ECS::ComponentTypeCounter::GetTypeId<TransformComponent>());
        controllerSignature.set(ECS::ComponentTypeCounter::GetTypeId<CameraControllerComponent>());
        world->SetSystemSignature<CameraControllerSystem>(controllerSignature);
    }
    
    ECS::Entity CreateCamera(const glm::vec3& position, const float fov = 45.0f, const float aspect = 16.0f/9.0f)
	{
        const ECS::Entity camera = world->CreateEntity();
        
        // Add transform
        world->AddComponent(camera, TransformComponent(position));
        
        // Add camera component
        world->AddComponent(camera, CameraComponent(fov, aspect));
        
        // Set as active if first camera
        if (activeCamera == 0)
        {
            activeCamera = camera;
        }
        
        return camera;
    }
    
    ECS::Entity CreateFpsCamera(const glm::vec3& position, const float fov = 60.0f, const float aspect = 16.0f/9.0f)
	{
        const ECS::Entity camera = CreateCamera(position, fov, aspect);
        
        // Add controller component
        world->AddComponent(camera, CameraControllerComponent());
        
        return camera;
    }
    
    void SetActiveCamera(ECS::Entity camera)
	{
        if (world->HasComponent<CameraComponent>(camera))
        {
            // Deactivate previous camera
            if (activeCamera != 0 && world->HasComponent<CameraComponent>(activeCamera))
            {
                world->GetComponent<CameraComponent>(activeCamera).isActive = false;
            }
            
            // Activate new camera
            activeCamera = camera;
            world->GetComponent<CameraComponent>(camera).isActive = true;
        }
    }
    
    ECS::Entity GetActiveCamera() const { return activeCamera; }
    
    const CameraSystem::CameraData& GetCameraData() const
	{
        return cameraSystem->GetCurrentCameraData();
    }
    
    void Update(float dt) const
	{
        world->Update(dt);
    }
    
    void OnEvent(Event& e) const
	{
        controllerSystem->OnEvent(e);
        
        // Handle window resize
        EventDispatcher dispatcher(e);
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e)
        {
            float aspect = static_cast<float>(e.GetWidth()) / static_cast<float>(e.GetHeight());
            cameraSystem->SetAspectRatio(aspect);
            return false;
        });
    }
    
    // Utility functions
    void SetCameraPosition(const ECS::Entity camera, const glm::vec3& position) const
	{
        if (world->HasComponent<TransformComponent>(camera))
        {
            world->GetComponent<TransformComponent>(camera).position = position;
        }
    }
    
    void SetCameraRotation(const ECS::Entity camera, const glm::vec3& eulerAngles) const
	{
        if (world->HasComponent<TransformComponent>(camera))
        {
            world->GetComponent<TransformComponent>(camera).SetEulerAngles(eulerAngles);
        }
    }
    
    void EnableCameraControl(const ECS::Entity camera, const bool enable) const
	{
        if (world->HasComponent<CameraControllerComponent>(camera))
        {
            world->GetComponent<CameraControllerComponent>(camera).isControllable = enable;
        }
    }
};