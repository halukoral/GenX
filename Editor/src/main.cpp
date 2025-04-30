#include "Application.h"
#include "EntryPoint.h"
#include <imgui.h>

class EditorLayer : public Layer
{
public:
    void OnUIRender() override
    {
        ImGui::Begin("Hello");
        ImGui::Button("Button");
        ImGui::End();
        
        ImGui::ShowDemoWindow();
    }

    void OnEvent(Event& e) override
    {
        
    }
};

Application* CreateApplication(int argc, char** argv)
{
    AppSpec spec;
    spec.Name = "Editor";

    auto app = new Application(spec);
    app->PushLayer<EditorLayer>();
    app->SetMenubarCallback([app]()
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
            {
                app->Close();
            }
            ImGui::EndMenu();
        }
    });
    return app;
}