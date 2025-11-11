#include "Game.h"
#include <GLFW/glfw3.h>
#include <iostream>

bool Game::Init()
{
    return true;
}

void Game::Update(float deltaTime)
{
    auto& input = eng::Engine::GetInstance().GetInputManager();

    if (input.IsKeyPressed(GLFW_KEY_A))
    {
        std::cout << "[A] button is pressed" << std::endl;
    }
}

void Game::Destroy()
{

}