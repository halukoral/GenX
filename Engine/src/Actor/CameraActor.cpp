#include "CameraActor.h"

#include "TimeStep.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/CameraController.h"
#include "ECS/Components/TransformComponent.h"

CameraActor::CameraActor() : Actor(s_Name + std::to_string(s_CameraActorCount++))
{
	m_CameraComponent = m_Entity->AddComponent<CameraComponent>();
	m_CameraControllerComponent = m_Entity->AddComponent<CameraController>();
	m_TransformComponent = m_Entity->AddComponent<TransformComponent>();

	m_CameraComponent->UpdateCameraVectors();
}

CameraActor::CameraActor(const std::string& name) : Actor(name)
{
	m_CameraComponent = m_Entity->AddComponent<CameraComponent>();
	m_CameraControllerComponent = m_Entity->AddComponent<CameraController>();
	m_TransformComponent = m_Entity->AddComponent<TransformComponent>();

	m_CameraComponent->UpdateCameraVectors();
}

CameraActor::~CameraActor()
{
	m_CameraComponent.reset();
	m_CameraControllerComponent.reset();
	m_TransformComponent.reset();
}

CameraActor::CameraActor(CameraActor& other)
{
	*m_CameraComponent = *other.m_CameraComponent;
	*m_CameraControllerComponent = *other.m_CameraControllerComponent;
	*m_TransformComponent = *other.m_TransformComponent;
	m_CameraComponent->UpdateCameraVectors();
}

CameraActor& CameraActor::operator=(CameraActor& other)
{
	*m_CameraComponent = *other.m_CameraComponent;
	*m_CameraControllerComponent = *other.m_CameraControllerComponent;
	*m_TransformComponent = *other.m_TransformComponent;
	m_CameraComponent->UpdateCameraVectors();
	return *this;
}

void CameraActor::OnUpdate(TimeStep deltaTime) const
{
	m_CameraControllerComponent->OnUpdate(deltaTime);
}

void CameraActor::OnEvent(Event& e) const
{
	m_CameraControllerComponent->OnEvent(e);
}

void CameraActor::OnResize(const uint32_t width, const uint32_t height) const
{
	m_CameraControllerComponent->OnResize(width, height);
}

void CameraActor::SetPosition(const glm::vec3 position) const
{
	m_TransformComponent->Position = position;
}

glm::mat4 CameraActor::GetViewMatrix() const
{
	return m_CameraComponent->GetViewMatrix();
}

glm::mat4 CameraActor::GetProjectionMatrix(const CameraType cameraType) const
{
	return m_CameraComponent->GetProjectionMatrix(cameraType);
}
