#pragma once
#include "CameraController.h"

class EditorCameraController : public CameraController
{
public:
	COMPONENT_CLASS_TYPE(EditorCameraController)
	
	EditorCameraController() = default;
	
	void OnUpdate(TimeStep deltaTime) override;
	void OnEvent(Event& e) override;
	bool OnMouseMoved(MouseMovedEvent& e) override;
};
