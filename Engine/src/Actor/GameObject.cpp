#include "GameObject.h"

GameObject GameObject::MakePointLight(const float intensity, const float radius, const glm::vec3 color)
{
	GameObject gameObj = GameObject::CreateGameObject();
	gameObj.Color = color;
	gameObj.Transform.Scale.x = radius;
	gameObj.PointLight = std::make_unique<PointLightComponent>();
	gameObj.PointLight->LightIntensity = intensity;
	return gameObj;
}