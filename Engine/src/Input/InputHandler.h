#pragma once

#include "pch.h"
#include "Renderer/Model.h"
#include <GLFW/glfw3.h>

class InputHandler
{
public:
	InputHandler(GLFWwindow* window, Camera* camera);
	~InputHandler() = default;

	void Update(float deltaTime);
	void ProcessKeyboard(float deltaTime);
	void ProcessMouse();
    
	// Mouse callback'leri için static fonksiyonlar
	static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    
	// Input durumları
	bool IsKeyPressed(int key) const;
	void SetMouseCaptured(bool captured);
	bool IsMouseCaptured() const { return m_MouseCaptured; }
    
	// Kamera kontrolü
	void SetCamera(Camera* camera) { m_Camera = camera; }
	void ResetCamera();

private:
	GLFWwindow* m_Window;
	Camera* m_Camera;
    
	// Mouse durumu
	bool m_MouseCaptured;
	bool m_FirstMouse;
	float m_LastX;
	float m_LastY;
    
	// Input durumları
	std::array<bool, GLFW_KEY_LAST> m_KeyStates{};
    
	static InputHandler* s_Instance; // Singleton pattern callbacks için
};
