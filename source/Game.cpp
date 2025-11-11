#include "Game.h"
#include <iostream>

bool Game::Init()
{
    return true;
}

void Game::Update(float deltaTime)
{
    std::cout << "Current delta: " << deltaTime << std::endl;
}

void Game::Destroy()
{

}