#include "InputHandler.h"
#include <iostream>

InputHandler* InputHandler::s_Instance = nullptr;

InputHandler::InputHandler(GLFWwindow* window, Camera* camera) 
    : m_Window(window), m_Camera(camera), m_MouseCaptured(false), m_FirstMouse(true), 
      m_LastX(0.0f), m_LastY(0.0f)
{
    s_Instance = this;
    
    // GLFW callback'leri ayarla
    glfwSetCursorPosCallback(m_Window, MouseCallback);
    glfwSetScrollCallback(m_Window, ScrollCallback);
    
    // Başlangıçta mouse'u yakalamayı ayarla
    SetMouseCaptured(true);
    
    std::cout << "Input Handler başlatıldı." << std::endl;
    std::cout << "Kontroller:" << std::endl;
    std::cout << "  WASD - Hareket" << std::endl;
    std::cout << "  QE - Yukarı/Aşağı" << std::endl;
    std::cout << "  Mouse - Bakış yönü" << std::endl;
    std::cout << "  Scroll - Zoom" << std::endl;
    std::cout << "  ESC - Mouse serbest bırak/yakala" << std::endl;
    std::cout << "  R - Kamerayı sıfırla" << std::endl;
}

void InputHandler::Update(float deltaTime)
{
    ProcessKeyboard(deltaTime);
    ProcessMouse();
}

void InputHandler::ProcessKeyboard(float deltaTime)
{
    if (!m_Camera) return;
    
    // Hareket tuşları
    if (IsKeyPressed(GLFW_KEY_W))
        m_Camera->processKeyboard(Camera::FORWARD, deltaTime);
    if (IsKeyPressed(GLFW_KEY_S))
        m_Camera->processKeyboard(Camera::BACKWARD, deltaTime);
    if (IsKeyPressed(GLFW_KEY_A))
        m_Camera->processKeyboard(Camera::LEFT, deltaTime);
    if (IsKeyPressed(GLFW_KEY_D))
        m_Camera->processKeyboard(Camera::RIGHT, deltaTime);
    if (IsKeyPressed(GLFW_KEY_Q))
        m_Camera->processKeyboard(Camera::UP, deltaTime);
    if (IsKeyPressed(GLFW_KEY_E))
        m_Camera->processKeyboard(Camera::DOWN, deltaTime);
    
    // ESC tuşu - mouse yakalama toggle
    static bool escPressed = false;
    if (IsKeyPressed(GLFW_KEY_ESCAPE)) {
        if (!escPressed) {
            SetMouseCaptured(!m_MouseCaptured);
            escPressed = true;
        }
    } else {
        escPressed = false;
    }
    
    // R tuşu - kamerayı sıfırla
    static bool rPressed = false;
    if (IsKeyPressed(GLFW_KEY_R)) {
        if (!rPressed) {
            ResetCamera();
            rPressed = true;
        }
    } else {
        rPressed = false;
    }
    
    // Hız ayarları (Shift - hızlı, Ctrl - yavaş)
    float speedMultiplier = 1.0f;
    if (IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
        speedMultiplier = 3.0f;
    else if (IsKeyPressed(GLFW_KEY_LEFT_CONTROL))
        speedMultiplier = 0.3f;
    
    m_Camera->setMovementSpeed(Camera::DEFAULT_SPEED * speedMultiplier);
}

void InputHandler::ProcessMouse()
{
    // Mouse yakalanmışsa cursor pozisyonunu al
    if (m_MouseCaptured) {
        double xpos, ypos;
        glfwGetCursorPos(m_Window, &xpos, &ypos);
        
        if (m_FirstMouse) {
            m_LastX = static_cast<float>(xpos);
            m_LastY = static_cast<float>(ypos);
            m_FirstMouse = false;
        }

        float xoffset = static_cast<float>(xpos) - m_LastX;
        float yoffset = m_LastY - static_cast<float>(ypos); // Y koordinatı tersine çevrilmiş
        
        m_LastX = static_cast<float>(xpos);
        m_LastY = static_cast<float>(ypos);

        if (m_Camera) {
            m_Camera->processMouseMovement(xoffset, yoffset);
        }
    }
}

void InputHandler::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Bu callback şu anda kullanılmıyor, ProcessMouse() kullanıyoruz
    // Ama gelecekte event-based sistem için kullanılabilir
}

void InputHandler::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (s_Instance && s_Instance->m_Camera) {
        s_Instance->m_Camera->processMouseScroll(static_cast<float>(yoffset));
    }
}

bool InputHandler::IsKeyPressed(int key) const
{
    return glfwGetKey(m_Window, key) == GLFW_PRESS;
}

void InputHandler::SetMouseCaptured(bool captured)
{
    m_MouseCaptured = captured;
    m_FirstMouse = true; // Mouse yakalama değiştiğinde ilk hareket atlanır
    
    if (captured) {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        std::cout << "Mouse yakalandı (FPS modu)" << std::endl;
    } else {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        std::cout << "Mouse serbest bırakıldı" << std::endl;
    }
}

void InputHandler::ResetCamera()
{
    if (m_Camera) {
        m_Camera->reset();
        std::cout << "Kamera sıfırlandı" << std::endl;
        m_Camera->printDebugInfo();
    }
}