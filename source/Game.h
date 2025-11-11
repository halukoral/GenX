#pragma once
#include <eng.h>

class Game : public eng::Application
{
public:
    bool Init() override;
    void Update(float deltaTime) override;
    void Destroy() override;
};