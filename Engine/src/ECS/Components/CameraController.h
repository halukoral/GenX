#pragma once
#include <glm/vec2.hpp>

#include "ECS/Component.h"

enum class CameraMovement;
class CameraComponent;
class Event;
class MouseScrolledEvent;
class MouseMovedEvent;
class TimeStep;
class WindowResizeEvent;

class CameraController : public Component
{
public:
	COMPONENT_CLASS_TYPE(CameraController)
	
	CameraController() = default;
	CameraController(const CameraController& other);
	CameraController& operator=(const CameraController& other);

	virtual void OnUpdate(TimeStep deltaTime);
	virtual void OnEvent(Event& e);
	virtual void OnMove(CameraMovement direction, TimeStep deltaTime) const;
	
	void OnResize(uint32_t width, uint32_t height);
	void SetPosition(CameraMovement direction, float velocity) const;

	[[nodiscard]] bool GetMouseFirstMove() const { return m_FirstMouseMove; }
	void SetMouseFirstMove(const float value) { m_FirstMouseMove = value; }

	[[nodiscard]] std::pair<float,float> GetMousePos() const { return {m_LastMouseX, m_LastMouseY}; }
	void SetMousePos(const std::pair<float,float> value) { m_LastMouseX = value.first; m_LastMouseY = value.second; }

	[[nodiscard]] float GetMovementSpeed() const { return m_MovementSpeed; }
	void SetMovementSpeed(const float value) { m_MovementSpeed = value; }

	[[nodiscard]] float GetPanSpeed() const { return m_PanSpeed; }
	void SetPanSpeed(const float value) { m_PanSpeed = value; }

	[[nodiscard]] float GetMouseSensitivity() const { return m_MouseSensitivity; }
	void SetMouseSensitivity(const float value) { m_MouseSensitivity = value; }

	[[nodiscard]] glm::vec2 GetViewportSize() const { return m_ViewportSize; }
	void SetViewportSize(const glm::vec2 value) { m_ViewportSize = value; }
	
protected:
	virtual bool OnMouseMoved(MouseMovedEvent& e);
	bool OnMousePanned(const MouseMovedEvent& e);
	bool OnMouseScrolled(const MouseScrolledEvent& e) const;
	bool OnWindowResized(const WindowResizeEvent& e) const;
	
protected:
	bool  m_FirstMouseMove = true;
	float m_LastMouseX;
	float m_LastMouseY;
	float m_MovementSpeed = 5.f;
	float m_PanSpeed = 0.025f;
	float m_MouseSensitivity = 0.1f;

	glm::vec2 m_ViewportSize;
};
