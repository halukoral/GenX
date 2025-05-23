#pragma once

#include "pch.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Event/Event.h"

using EventCallbackFn = std::function<void(Event&)>;

struct WindowAttributes
{
	bool VSync;
	std::string Title;
	uint32_t Width;
	uint32_t Height;
	EventCallbackFn EventCallback;

	WindowAttributes(const std::string& title = "Genix Engine",
					 bool vSync = true,
					 uint32_t width = 2560,
					 uint32_t height = 1440)
		: Title(title), Width(width), Height(height), VSync(vSync)
	{
	}
};

class Window
{
public:
	using EventCallbackFn = std::function<void(Event&)>;
	
	Window(uint32_t w, uint32_t h, const std::string& name);
	~Window();

	Window(const Window &) = delete;
	Window &operator=(const Window &) = delete;

	GLFWwindow* GetWindow() const { return m_Window; }
	
	void InitializeWindow();

	bool ShouldClose() const { return glfwWindowShouldClose(m_Window); }
	bool WasWindowResized() const { return framebufferResized; }
	void ResetWindowResizedFlag() { framebufferResized = false; }
	
	void CreateWindowSurface(VkInstance instance, VkSurfaceKHR *surface) const;

	int GetWidth() const { return width; }
	int GetHeight() const { return height; }

	VkExtent2D GetExtent() const
	{
		return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
	}

	const WindowAttributes& GetWindowAttributes() const { return m_Data; }
	void SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }
	void EnableCursor() const;
	void DisableCursor() const;

protected:
	WindowAttributes m_Data;
	
private:
	static void FramebufferResizeCallback(GLFWwindow *window, int width, int height);

private:
	uint32_t width;
	uint32_t height;
	std::string windowName;
	bool framebufferResized = false;
	EventCallbackFn eventCallback;
	
	GLFWwindow* m_Window;  	
};
