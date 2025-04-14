#pragma once

#include <optional>

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
	void SetupDebugMessenger();
	void PickPhysicalDevice();

	static gsl::span<gsl::czstring> GetSuggestedInstanceExtensions();
	static std::vector<gsl::czstring> GetRequiredInstanceExtensions();
	static std::vector<VkExtensionProperties> GetSupportedInstanceExtensions();
	static std::vector<VkLayerProperties> GetSupportedValidationLayers();
	static bool AreAllExtensionsSupported(const gsl::span<gsl::czstring>& extensions);
	static bool AreAllLayersSupported(gsl::span<gsl::czstring> layers);

	bool IsDeviceSuitable(VkPhysicalDevice device);
	std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices() const;

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily = std::nullopt;
		std::optional<uint32_t> presentationFamily = std::nullopt;

		[[nodiscard]] bool IsValid() const
		{
			return graphicsFamily.has_value() ; // && presentationFamily.has_value();
		}
	};

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	
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
	VkDebugUtilsMessengerEXT m_DebugMessenger;
	VkPhysicalDevice m_PhysicalDevice;

	
};

// Implemented by CLIENT
Application* CreateApplication(int argc, char** argv);