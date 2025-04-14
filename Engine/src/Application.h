#pragma once

#include "pch.h"
#include "Layer.h"
#include "vulkan/vulkan.h"

void CheckVkResult(VkResult err);

struct GLFWwindow;

struct AppSpec
{
	std::string Name = "Engine";
	uint32_t Width = 2560;
	uint32_t Height = 1440;
};

class Application
{
public:
	Application(AppSpec spec = AppSpec());
	~Application();

	static Application& Get();

	void Run();
	void SetMenubarCallback(const std::function<void()>& menubarCallback) { m_MenubarCallback = menubarCallback; }
		
	template<typename T>
	void PushLayer()
	{
		static_assert(std::is_base_of<Layer, T>::value, "Pushed type is not subclass of Layer!");
		m_LayerStack.emplace_back(std::make_shared<T>())->OnAttach();
	}

	void PushLayer(const std::shared_ptr<Layer>& layer)
	{
		m_LayerStack.emplace_back(layer); layer->OnAttach();
	}

	void Close();
	static float GetTime();
	GLFWwindow* GetWindowHandle() const { return m_Window; }
	
private:
	void Init();
	bool InitVulkan();
	void Shutdown();

	void CreateInstance();

	static gsl::span<gsl::czstring> GetSuggestedInstanceExtensions();
	static std::vector<gsl::czstring> GetRequiredInstanceExtensions();
	static std::vector<VkExtensionProperties> GetSupportedInstanceExtensions();
	static bool AreAllExtensionsSupported(const gsl::span<gsl::czstring>& extensions);

	static std::vector<VkLayerProperties> GetSupportedValidationLayers();
	static bool AreAllLayersSupported(gsl::span<gsl::czstring> layers);
	void SetupDebugMessenger();

private:
	
	AppSpec m_Spec;
	GLFWwindow* m_Window = nullptr;
	bool m_Running = false;

	float m_TimeStep = 0.0f;
	float m_FrameTime = 0.0f;
	float m_LastFrameTime = 0.0f;

	std::vector<std::shared_ptr<Layer>> m_LayerStack;
	std::function<void()> m_MenubarCallback;

	/////////////////////////////////////////////////
	/// VULKAN
	
	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT debugMessenger;
};

// Implemented by CLIENT
Application* CreateApplication(int argc, char** argv);