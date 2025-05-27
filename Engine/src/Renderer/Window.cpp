#include "Window.h"

#include "Event/ApplicationEvent.h"

void FramebufferSizeCallback(GLFWwindow* window, int32_t width, int32_t height)
{
	WindowAttributes& data = *(WindowAttributes*)glfwGetWindowUserPointer(window);
	data.Width = width;
	data.Height = height;

	WindowResizeEvent event(width, height);
	data.EventCallback(event);	
}

void WindowCloseCallback(GLFWwindow* window)
{
	const WindowAttributes& data = *(WindowAttributes*)glfwGetWindowUserPointer(window);
	WindowCloseEvent event;
	data.EventCallback(event);
}


void GlfwErrorCallback(int error, const char* description)
{
	//spdlog::error ("Glfw Validation: {}", description);
}

Window::Window(const uint32_t w, const uint32_t h, const std::string& name): width(w), height(h), windowName(name)
{
	InitializeWindow();
}

Window::~Window()
{
	glfwDestroyWindow(m_Window);
	glfwTerminate();
}

void Window::InitializeWindow()
{
	// Setup GLFW window
	glfwSetErrorCallback(GlfwErrorCallback);
	if (!glfwInit())
	{
		//spdlog::error("Failed to initialize GLFW!");
		return;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_Window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(m_Window, &m_Data);
	
	// Set GLFW callbacks
	glfwSetWindowSizeCallback(m_Window, FramebufferSizeCallback);
	glfwSetWindowCloseCallback(m_Window, WindowCloseCallback);

}

void Window::CreateWindowSurface(const VkInstance instance, VkSurfaceKHR *surface) const
{
	if (glfwCreateWindowSurface(instance, m_Window, nullptr, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void Window::EnableCursor() const
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::DisableCursor() const
{
	glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::FramebufferResizeCallback(GLFWwindow* window, int width, int height)
{
	const auto window_ = static_cast<Window*>(glfwGetWindowUserPointer(window));
	window_->framebufferResized = true;
	window_->width = width;
	window_->height = height;
}
