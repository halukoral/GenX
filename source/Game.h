#pragma once
#include <eng.h>
#include <memory>

class Game : public eng::Application
{
public:
    bool Init() override;
    void Update(float deltaTime) override;
    void Destroy() override;

private:
    eng::Material m_material;
    std::unique_ptr<eng::Mesh> m_mesh;
};