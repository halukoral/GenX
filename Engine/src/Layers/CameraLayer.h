#pragma once

#include "Layer.h"
#include "ECS/ECS.h"
#include "ECS/Systems/CameraSystem.h"

// ========== INTEGRATION LAYER ==========

// Layer for integrating ECS camera with existing engine
class CameraLayer : public Layer
{
private:
	std::unique_ptr<ECS::World> ecsWorld;
	std::unique_ptr<CameraManager> cameraManager;
    
public:
	CameraLayer();

	void OnAttach() override;

	void OnDetach() override;

	void OnUpdate(float ts) override;

	void OnEvent(Event& event) override;

	const CameraSystem::CameraData& GetCameraData() const;

	CameraManager* GetCameraManager() const { return cameraManager.get(); }
};