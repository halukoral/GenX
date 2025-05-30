#include "CameraLayer.h"

#include "Application.h"

CameraLayer::CameraLayer()
{
	ecsWorld = std::make_unique<ECS::World>();
	cameraManager = std::make_unique<CameraManager>(ecsWorld.get());
}

void CameraLayer::OnAttach()
{
	// Create default camera
	auto camera = cameraManager->createFPSCamera(glm::vec3(0.0f, 5.0f, 0.0f));
	cameraManager->setActiveCamera(camera);
        
	// Set initial aspect ratio from window
	auto& app = Application::Get();
	int width, height;
	app.GetWindow()->getFramebufferSize(&width, &height);
	float aspect = static_cast<float>(width) / static_cast<float>(height);
        
	if (auto* world = ecsWorld.get())
	{
		world->getComponent<CameraComponent>(camera).aspectRatio = aspect;
	}
}

void CameraLayer::OnDetach()
{
	cameraManager.reset();
	ecsWorld.reset();
}

void CameraLayer::OnUpdate(float ts)
{
	cameraManager->update(ts);
}

void CameraLayer::OnEvent(Event& event)
{
	cameraManager->onEvent(event);
}

const CameraSystem::CameraData& CameraLayer::GetCameraData() const
{
	return cameraManager->getCameraData();
}
