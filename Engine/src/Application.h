#pragma once

#include "pch.h"
#include "Layer.h"
#include <glm/glm.hpp>

#include "Event/ApplicationEvent.h"
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
	const std::shared_ptr<Window>& GetWindow() const { return m_Window; }
	const Renderer* GetRenderer() const { return m_Renderer.get(); }

	ModelLayer* GetModelLayer() const
	{ 
		return m_Renderer ? m_Renderer->GetModelLayer() : nullptr; 
	}
	
private:
	bool OnWindowClose(WindowCloseEvent& e);
	void Init();
	void Shutdown();

private:	
	AppSpec m_Spec;
	bool m_Running = false;

	float m_TimeStep = 0.0f;
	float m_FrameTime = 0.0f;
	float m_LastFrameTime = 0.0f;

	static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	
	std::vector<std::shared_ptr<Layer>> m_LayerStack;
	std::function<void()> m_MenubarCallback;
	
	/* --------------------------------------------------------------------*/

	std::shared_ptr<Window> m_Window = std::make_shared<Window>(m_Spec.Width, m_Spec.Height, m_Spec.Name);
	std::unique_ptr<Renderer> m_Renderer = std::make_unique<Renderer>(m_Window);
	
	VkDescriptorPool m_ImGuiDescriptorPool = VK_NULL_HANDLE;
};
