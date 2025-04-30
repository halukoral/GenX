#include "EditorCameraController.h"

#include "CameraComponent.h"
#include "TimeStep.h"
#include "Event/Event.h"
#include "Event/MouseEvent.h"
#include "Input/Input.h"

void EditorCameraController::OnUpdate(TimeStep deltaTime)
{
	if (Input::IsMouseButtonDown(MouseButton::Middle) || Input::IsMouseButtonDown(MouseButton::Right))
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

		if (Input::IsKeyDown(KeyCode::Q))
		{
			OnMove(CameraMovement::Down, deltaTime);
		}
		else if (Input::IsKeyDown(KeyCode::E))
		{
			OnMove(CameraMovement::Up, deltaTime);
		}
	}
}

void EditorCameraController::OnEvent(Event& e)
{
	EventDispatcher dispatcher(e);
	dispatcher.Dispatch<MouseMovedEvent>(GX_BIND(EditorCameraController::OnMouseMoved));
}

bool EditorCameraController::OnMouseMoved(MouseMovedEvent& e)
{
	if (Input::IsMouseButtonDown(MouseButton::Right))
	{
		return CameraController::OnMouseMoved(e);
	}
	else if (Input::IsMouseButtonDown(MouseButton::Middle))
	{
		return CameraController::OnMousePanned(e);
	}
	else
	{
		m_LastMouseX = e.GetX();
		m_LastMouseY = e.GetY();
	}
	return false;
}
