#pragma once

#include "pch.h"
#include "Layer.h"
#include <glm/glm.hpp>

#include "Actor/CameraActor.h"
#include "Actor/GameObject.h"
#include "Event/ApplicationEvent.h"
#include "Renderer/Descriptor.h"
#include "Renderer/Device.h"
#include "Renderer/Renderer.h"
#include "Renderer/Window.h"

class ApplicationLayer;
class CameraActor;
struct Vertex;

struct GLFWwindow;

struct AppSpec
{
	std::string Name = "GenX Vulkan Engine";
	uint32_t Width = 2560;
	uint32_t Height = 1440;
};

struct UniformBufferObject
{
	glm::mat4 Model;
	glm::mat4 View;
	glm::mat4 Projection;
};

class Application
{
private:
	struct SwapChainProperties
	{
		VkSurfaceCapabilitiesKHR Capabilities;
		std::vector<VkSurfaceFormatKHR> Formats;
		std::vector<VkPresentModeKHR> PresentModes;

		[[nodiscard]] bool IsValid() const
		{
			return !Formats.empty() && !PresentModes.empty();
		}
	};
	
public:
	friend ApplicationLayer;
	
	Application(AppSpec spec = AppSpec());
	~Application();

	static Application& Get();

	void OnEvent(Event& e);

	void Run();
	void SetMenubarCallback(const std::function<void()>& menubarCallback) { m_MenubarCallback = menubarCallback; }
		
	template<typename T>
	void PushLayer()
	{
		static_assert(std::is_base_of_v<Layer, T>, "Pushed type is not subclass of Layer!");
		m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
	}

	void PushLayer(const std::shared_ptr<Layer>& layer)
	{
		m_LayerStack.emplace_back(layer); layer->OnAttach();
	}

	void			Close();
	static float	GetTime();
	GLFWwindow*		GetWindowHandle() const { return m_Window->GetWindow(); }
	CameraActor*	GetCameraActor() { return &m_CameraActor; }

private:
	bool OnWindowClose(WindowCloseEvent& e);
	void Init();
	bool InitVulkan();
	void Shutdown();

	void InitImgui();
	void CleanupImGui() const;
	void CreateImGuiDescriptorPool();

	void LoadGameObjects();

	
protected:
	CameraActor m_CameraActor;

private:	
	AppSpec m_Spec;
	bool m_Running = false;

	float m_TimeStep = 0.0f;
	float m_FrameTime = 0.0f;
	float m_LastFrameTime = 0.0f;

	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	
	Ref<ApplicationLayer> m_Layer = nullptr;
	std::vector<std::shared_ptr<Layer>> m_LayerStack;
	std::function<void()> m_MenubarCallback;
	
	/* --------------------------------------------------------------------*/

	std::shared_ptr<Window> m_Window = std::make_shared<Window>(m_Spec.Width, m_Spec.Height, m_Spec.Name);
	std::shared_ptr<Device> m_Device = std::make_shared<Device>(m_Window);
	std::shared_ptr<Renderer> m_Renderer = std::make_shared<Renderer>(m_Window, m_Device);
	
	// note: order of declarations matters
	std::unique_ptr<DescriptorPool> m_GlobalPool{};
	GameObject::Map m_GameObjects;
	
	VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
};

// Implemented by CLIENT
Application* CreateApplication(int argc, char** argv);

class ApplicationLayer : public Layer
{
public:
	void OnEvent(Event& e) override
	{
		if (cameraActor)
		{
			cameraActor->OnEvent(e);
		}
	}

	void SetCamera(Application* app) { cameraActor = app->GetCameraActor(); }
	
private:
	CameraActor* cameraActor = nullptr;	
};