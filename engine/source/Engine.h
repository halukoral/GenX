#pragma once
#include <memory>
#include <chrono>

namespace eng
{
    class Application;
    class Engine
    {
    public:
        bool Init();
        void Run();
        void Destroy();

        void SetApplication(Application* app);
        Application* GetApplication();

    private:
        std::unique_ptr<Application> m_application;
        std::chrono::steady_clock::time_point m_lastTimePoint;
    };
}