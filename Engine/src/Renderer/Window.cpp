#include "Window.h"

#include "Event/ApplicationEvent.h"
#include "Event/KeyEvent.h"
#include "Event/MouseEvent.h"
#include "Input/KeyCodes.h"
#include "spdlog/spdlog.h"

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

void MouseButtonCallback(GLFWwindow* window, int32_t button, int32_t action, int32_t mods)
{
	const WindowAttributes& data = *(WindowAttributes*)glfwGetWindowUserPointer(window);
	
	switch (action)
	{
		case GLFW_PRESS:
		{
			MouseButtonPressedEvent event((MouseButton)button);
			data.EventCallback(event);
			break;
		}
		case GLFW_RELEASE:
		{
			MouseButtonReleasedEvent event((MouseButton)button);
			data.EventCallback(event);
			break;
		}
	}	
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
	WindowAttributes& data = *(WindowAttributes*)glfwGetWindowUserPointer(window);

	MouseMovedEvent event((float)xPos, (float)yPos);
	data.EventCallback(event);
}

void ScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	WindowAttributes& data = *(WindowAttributes*)glfwGetWindowUserPointer(window);

	MouseScrolledEvent event((float)xOffset, (float)yOffset);
	data.EventCallback(event);	
}

void KeyCallback(GLFWwindow* window, int32_t key, int32_t scancode, int32_t action, int32_t mods)
{
	WindowAttributes& data = *(WindowAttributes*)glfwGetWindowUserPointer(window);
	
	switch (action)
	{
	case GLFW_PRESS:
	{
		KeyPressedEvent event((KeyCode)(KeyCode)key, 0);
		data.EventCallback(event);
		break;
	}
	case GLFW_RELEASE:
	{
		KeyReleasedEvent event((KeyCode)key);
		data.EventCallback(event);
		break;
	}
	case GLFW_REPEAT:
	{
		KeyPressedEvent event((KeyCode)key, true);
		data.EventCallback(event);
		break;
	}
	default:
		break;
	}	
}

void CharCallback(GLFWwindow* window, uint32_t keycode)
{
	WindowAttributes& data = *(WindowAttributes*)glfwGetWindowUserPointer(window);

	KeyTypedEvent event((KeyCode)keycode);
	data.EventCallback(event);
}

void GlfwErrorCallback(int error, const char* description)
{
	spdlog::error ("Glfw Validation: {}", description);
}

Window::Window(uint32_t w, uint32_t h, std::string name): width(w), height(h), windowName(name)
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
		spdlog::error("Failed to initialize GLFW!");
		return;
	}
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_Window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
	glfwSetWindowUserPointer(m_Window, &m_Data);
	
	// Set GLFW callbacks
	glfwSetWindowSizeCallback(m_Window, FramebufferSizeCallback);
	glfwSetWindowCloseCallback(m_Window, WindowCloseCallback);

	glfwSetKeyCallback(m_Window, KeyCallback);
	glfwSetCharCallback(m_Window, CharCallback);
	
	glfwSetMouseButtonCallback(m_Window, MouseButtonCallback);
	glfwSetScrollCallback(m_Window, ScrollCallback);
	glfwSetCursorPosCallback(m_Window, MouseCallback);
	
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
	auto window_ = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
	window_->framebufferResized = true;
	window_->width = width;
	window_->height = height;
}
