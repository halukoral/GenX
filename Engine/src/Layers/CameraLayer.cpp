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
	auto camera = cameraManager->CreateFpsCamera(glm::vec3(0.0f, 0.0f, 0.0f));
	cameraManager->SetActiveCamera(camera);
        
	// Set initial aspect ratio from window
	auto& app = Application::Get();
	int width, height;
	app.GetWindow()->getFramebufferSize(&width, &height);
	float aspect = static_cast<float>(width) / static_cast<float>(height);
        
	if (auto* world = ecsWorld.get())
	{
		world->GetComponent<CameraComponent>(camera).aspectRatio = aspect;
	}
}

void CameraLayer::OnDetach()
{
	cameraManager.reset();
	ecsWorld.reset();
}

void CameraLayer::OnUpdate(float ts)
{
	cameraManager->Update(ts);
}

void CameraLayer::OnEvent(Event& event)
{
	cameraManager->OnEvent(event);
}

const CameraSystem::CameraData& CameraLayer::GetCameraData() const
{
	return cameraManager->GetCameraData();
}
