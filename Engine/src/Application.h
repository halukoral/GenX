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

	void PushLayer(const std::shared_ptr<Layer>& layer) { m_LayerStack.emplace_back(layer); layer->OnAttach(); }

	void Close();

	float GetTime();
	GLFWwindow* GetWindowHandle() const { return m_WindowHandle; }

private:
	void Init();
	bool InitWindow();
	void Shutdown();

	bool InitVulkan();
	void CreateInstance();
	void SetupDebugMessenger();
	void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
	void PickPhysicalDevice();
	bool IsDeviceSuitable(VkPhysicalDevice device);

private:
	GLFWwindow* m_WindowHandle {};

	// First thing you need to initialize the Vulkan by VkInstance.
	// The starting point of Vulkan app.
	// It represents a connection between app and Vulkan library.
	VkInstance m_Instance {};
	VkDebugUtilsMessengerEXT debugMessenger;

	// a handle to a physical device in Vulkan
	// You must first create a VkInstance, then use
	// vkEnumeratePhysicalDevices() to get available one
	VkPhysicalDevice m_PhysicalDevice {};
	
	VkSurfaceKHR m_Surface {};
	
	AppSpec m_Spec;
	bool m_Running = false;

	float m_TimeStep = 0.0f;
	float m_FrameTime = 0.0f;
	float m_LastFrameTime = 0.0f;

	std::vector<std::shared_ptr<Layer>> m_LayerStack;
	std::function<void()> m_MenubarCallback;
};

// Implemented by CLIENT
Application* CreateApplication(int argc, char** argv);