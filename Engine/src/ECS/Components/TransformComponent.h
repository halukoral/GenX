#pragma once
#include "ECS/ECS.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

struct TransformComponent : public ECS::Component
{
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion
	glm::vec3 scale = glm::vec3(1.0f);
    
	// Euler angles for easier manipulation
	glm::vec3 eulerAngles = glm::vec3(0.0f);
    
	TransformComponent() = default;
	TransformComponent(const glm::vec3& pos) : position(pos) {}
	TransformComponent(const glm::vec3& pos, const glm::vec3& rot, const glm::vec3& scl)
		: position(pos), eulerAngles(rot), scale(scl) {
		UpdateRotationFromEuler();
	}
    
	void UpdateRotationFromEuler() {
		rotation = glm::quat(glm::radians(eulerAngles));
	}
    
	void SetEulerAngles(const glm::vec3& angles) {
		eulerAngles = angles;
		UpdateRotationFromEuler();
	}
    
	glm::mat4 GetTransformMatrix() const {
		return glm::translate(glm::mat4(1.0f), position) *
			   glm::mat4_cast(rotation) *
			   glm::scale(glm::mat4(1.0f), scale);
	}
    
	glm::vec3 GetForward() const {
		return rotation * glm::vec3(0.0f, 0.0f, -1.0f);
	}
    
	glm::vec3 GetRight() const {
		return rotation * glm::vec3(1.0f, 0.0f, 0.0f);
	}
    
	glm::vec3 GetUp() const {
		return rotation * glm::vec3(0.0f, 1.0f, 0.0f);
	}
};
