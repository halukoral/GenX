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
private:
	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily = std::nullopt;
		std::optional<uint32_t> presentationFamily = std::nullopt;

		[[nodiscard]] bool IsValid() const
		{
			return graphicsFamily.has_value() && presentationFamily.has_value();
		}
	};

	struct SwapChainProperties
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		[[nodiscard]] bool IsValid() const
		{
			return !formats.empty() && !presentModes.empty();
		}
	};
	
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
	void CreateSurface();
	void PickPhysicalDevice();
	void CreateLogicalDeviceAndQueues();
	void CreateSwapChain();
	void CreateImageViews();
	void CreateRenderPass();
	void CreateGraphicsPipeline();
	void CreateFramebuffers();
	
	static std::vector<gsl::czstring> GetRequiredInstanceExtensions();

	static std::vector<VkLayerProperties> GetSupportedValidationLayers();
	static bool AreAllLayersSupported(gsl::span<gsl::czstring> layers);

	static gsl::span<gsl::czstring> GetSuggestedInstanceExtensions();
	static std::vector<VkExtensionProperties> GetSupportedInstanceExtensions();
	static bool AreAllExtensionsSupported(const gsl::span<gsl::czstring>& extensions);

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
	bool IsDeviceSuitable(VkPhysicalDevice device);
	std::vector<VkPhysicalDevice> GetAvailablePhysicalDevices() const;
	bool AreAllDeviceExtensionsSupported(VkPhysicalDevice device);
	std::vector<VkExtensionProperties> GetAvailableDeviceExtensions(VkPhysicalDevice device);

	// Swap chain functions
	SwapChainProperties GetSwapChainProperties(VkPhysicalDevice device) const;
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(gsl::span<VkSurfaceFormatKHR> formats);
	VkPresentModeKHR ChooseSwapPresentMode(gsl::span<VkPresentModeKHR> modes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	uint32_t ChooseSwapImageCount(const VkSurfaceCapabilitiesKHR& capabilities);

	// Graphics Pipeline
	VkShaderModule CreateShaderModule(gsl::span<uint8_t> buffer) const;
	VkViewport GetViewport() const;
	VkRect2D GetScissor() const;
	
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

	// 1
	VkInstance m_Instance = VK_NULL_HANDLE;
	VkDebugUtilsMessengerEXT m_DebugMessenger;

	// 2
	// After selecting physical device, you need to
	// set up a logical device to interface it.
	VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

	// 3
	VkDevice m_LogicalDevice = VK_NULL_HANDLE;

	// 4
	VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
	VkQueue m_PresentQueue = VK_NULL_HANDLE;

	// 5
	VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

	// 6
	VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
	VkSurfaceFormatKHR m_SurfaceFormat;
	VkPresentModeKHR m_PresentMode;
	VkExtent2D m_Extent;
	
	std::vector<VkImage> m_SwapChainImages;
	std::vector<VkImageView> m_SwapChainImageViews;

	// 7
	VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
	VkRenderPass m_RenderPass = VK_NULL_HANDLE;
	VkPipeline m_Pipeline = VK_NULL_HANDLE;

	// 8
	std::vector<VkFramebuffer> m_SwapChainFramebuffers;
	
	std::array<gsl::czstring, 1> m_RequiredDeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
};

// Implemented by CLIENT
Application* CreateApplication(int argc, char** argv);