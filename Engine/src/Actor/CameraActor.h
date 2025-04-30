#pragma once
#include <glm/fwd.hpp>

#include "Actor.h"

struct TransformComponent;
class CameraComponent;
class TimeStep;
enum class CameraType;
class Event;
class CameraController;

class CameraActor : public Actor
{
public:
	CameraActor();
	~CameraActor() override;
	CameraActor(CameraActor& other);
	CameraActor(const std::string& name);

	CameraActor& operator=(CameraActor& other);
	
	void OnUpdate(TimeStep deltaTime) const;
	void OnEvent(Event& e) const;
	void OnResize(uint32_t width, uint32_t height) const;

	void SetPosition(glm::vec3 position) const;

	glm::mat4 GetViewMatrix() const;
	glm::mat4 GetProjectionMatrix(CameraType cameraType) const;

private:
	inline static uint32_t s_CameraActorCount = 0;
	inline static const std::string s_Name = "CameraActor_";

	Ref<CameraComponent> m_CameraComponent;
	Ref<TransformComponent> m_TransformComponent;
	Ref<CameraController> m_CameraControllerComponent;
};
