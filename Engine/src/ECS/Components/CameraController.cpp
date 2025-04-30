#include "CameraController.h"
#include "CameraComponent.h"
#include "TransformComponent.h"
#include "TimeStep.h"

#include "ECS/Entity.h"
#include "Event/ApplicationEvent.h"
#include "Event/Event.h"
#include "Event/MouseEvent.h"
#include "Input/Input.h"

class TransformComponent;

CameraController::CameraController(const CameraController& other) : Component(other)
{
	m_FirstMouseMove = other.m_FirstMouseMove;
	m_LastMouseX = other.m_LastMouseX;
	m_LastMouseY = other.m_LastMouseY;
	m_MovementSpeed = other.m_MovementSpeed;
	m_PanSpeed = other.m_PanSpeed;
	m_MouseSensitivity = other.m_MouseSensitivity;

	m_ViewportSize = other.m_ViewportSize;
}

CameraController& CameraController::operator=(const CameraController& other)
{
	m_Entity = other.m_Entity;

	m_FirstMouseMove = other.m_FirstMouseMove;
	m_LastMouseX = other.m_LastMouseX;
	m_LastMouseY = other.m_LastMouseY;
	m_MovementSpeed = other.m_MovementSpeed;
	m_PanSpeed = other.m_PanSpeed;
	m_MouseSensitivity = other.m_MouseSensitivity;

	m_ViewportSize = other.m_ViewportSize;
	return *this;
}

void CameraController::OnUpdate(const TimeStep deltaTime)
{
	if (Input::IsKeyDown(KeyCode::A))
	{
		OnMove(CameraMovement::Left, deltaTime);
	}
	else if (Input::IsKeyDown(KeyCode::D))
	{
		OnMove(CameraMovement::Right, deltaTime);
	}

	if (Input::IsKeyDown(KeyCode::W))
	{
		OnMove(CameraMovement::Forward, deltaTime);
	}
	else if (Input::IsKeyDown(KeyCode::S))
	{
		OnMove(CameraMovement::Backward, deltaTime);
	}
	else if (Input::IsKeyDown(KeyCode::Q))
	{
		OnMove(CameraMovement::Up, deltaTime);
	}
	else if (Input::IsKeyDown(KeyCode::E))
	{
		OnMove(CameraMovement::Down, deltaTime);
	}
}

void CameraController::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<MouseMovedEvent>(GX_BIND(CameraController::OnMouseMoved));
	dispatcher.Dispatch<MouseScrolledEvent>(GX_BIND(CameraController::OnMouseScrolled));
	dispatcher.Dispatch<WindowResizeEvent>(GX_BIND(CameraController::OnWindowResized));
}


void CameraController::OnMove(const CameraMovement direction, const TimeStep deltaTime) const
{
	const float velocity = m_MovementSpeed * deltaTime;
	SetPosition(direction, velocity);
}

void CameraController::OnResize(const uint32_t width, const uint32_t height)
{
	m_ViewportSize.x = width;
	m_ViewportSize.y = height;
	if (const auto& cmp = GetEntity()->GetComponent<CameraComponent>())
	{
		cmp->SetAspectRatio((float)width / (float)height);
	}
}

void CameraController::SetPosition(const CameraMovement direction, const float velocity) const
{
	if (const auto& cmp = GetEntity()->GetComponent<TransformComponent>())
	{
		if (direction == CameraMovement::Up)
		{
			cmp->Position += cmp->Up * velocity;
		}
		if (direction == CameraMovement::Down)
		{
			cmp->Position -= cmp->Up * velocity;
		}
		if (direction == CameraMovement::Forward)
		{
			cmp->Position += cmp->Front * velocity;
		}
		if (direction == CameraMovement::Backward)
		{
			cmp->Position -= cmp->Front * velocity;
		}
		if (direction == CameraMovement::Left)
		{
			cmp->Position -= cmp->Right * velocity;
		}
		if (direction == CameraMovement::Right)
		{
			cmp->Position += cmp->Right * velocity;
		}
	}
}

bool CameraController::OnMouseMoved(MouseMovedEvent& e)
{
	if (m_FirstMouseMove)
	{
		m_LastMouseX = e.GetX();	
		m_LastMouseY = e.GetY();
		m_FirstMouseMove = false;
	}
	
	float xOffset = e.GetX() - m_LastMouseX;
	float yOffset = e.GetY() - m_LastMouseY;
	
	m_LastMouseX = e.GetX();
	m_LastMouseY = e.GetY();
	
	xOffset *= m_MouseSensitivity;
	yOffset *= m_MouseSensitivity;

	if (const auto& cmp = GetEntity()->GetComponent<CameraComponent>())
	{
		cmp->SetYaw(xOffset);
		cmp->SetPitch(yOffset);
		cmp->UpdateCameraVectors();
	}

	return false;
}

bool CameraController::OnMousePanned(const MouseMovedEvent& e)
{
	if (m_FirstMouseMove)
	{
		m_LastMouseX = e.GetX();	
		m_LastMouseY = e.GetY();
		m_FirstMouseMove = false;
	}
	
	float xOffset = e.GetX() - m_LastMouseX;
	float yOffset = e.GetY() - m_LastMouseY;

	m_LastMouseX = e.GetX();
	m_LastMouseY = e.GetY();

	xOffset *= m_PanSpeed;
	yOffset *= m_PanSpeed;

	SetPosition(CameraMovement::Right, xOffset);
	SetPosition(CameraMovement::Up, yOffset);
	
	return false;
}

bool CameraController::OnMouseScrolled(const MouseScrolledEvent& e) const
{
	if (const auto& cmp = GetEntity()->GetComponent<CameraComponent>())
	{
		cmp->SetFOV(e.GetYOffset() * 0.25f * -1.f);
	}
	
	return false;
}

bool CameraController::OnWindowResized(const WindowResizeEvent& e) const
{
	if (const auto& cmp = GetEntity()->GetComponent<CameraComponent>())
	{
		cmp->SetAspectRatio((float)e.GetWidth() / (float)e.GetHeight());
	}
	return false;
}
