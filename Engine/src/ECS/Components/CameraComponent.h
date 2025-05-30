#pragma once
#include "ECS/ECS.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


// Camera Component - Camera-specific properties
struct CameraComponent : public ECS::Component
{
	float fov = 45.0f;
	float nearPlane = 0.1f;
	float farPlane = 100.0f;
	float aspectRatio = 16.0f / 9.0f;
	bool isActive = true;
    
	// Camera type
	enum Type { PERSPECTIVE, ORTHOGRAPHIC };
	Type type = PERSPECTIVE;
    
	// For orthographic cameras
	float orthoSize = 10.0f;
    
	CameraComponent() = default;
	CameraComponent(float fieldOfView, float aspect, float near = 0.1f, float far = 100.0f)
		: fov(fieldOfView), aspectRatio(aspect), nearPlane(near), farPlane(far) {}
    
	glm::mat4 GetProjectionMatrix() const
	{
		if (type == PERSPECTIVE)
		{
			return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
		}
		else
		{
			float halfSize = orthoSize * 0.5f;
			float halfWidth = halfSize * aspectRatio;
			return glm::ortho(-halfWidth, halfWidth, -halfSize, halfSize, nearPlane, farPlane);
		}
	}
};

// Camera Controller Component - For input handling
struct CameraControllerComponent : public ECS::Component
{
	// Movement
	float moveSpeed = 5.0f;
	float sprintMultiplier = 2.0f;
    
	// Look
	float mouseSensitivity = 0.1f;
	float maxPitch = 89.0f;
    
	// State
	glm::vec2 lastMousePos = glm::vec2(0.0f);
	bool firstMouse = true;
	bool isControllable = true;
    
	// Smoothing
	float smoothTime = 0.1f;
	glm::vec3 velocitySmoothing = glm::vec3(0.0f);
    
	CameraControllerComponent() = default;
};