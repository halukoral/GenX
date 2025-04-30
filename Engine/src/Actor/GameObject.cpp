#include "GameObject.h"

GameObject GameObject::makePointLight(float intensity, float radius, glm::vec3 color)
{
	GameObject gameObj = GameObject::createGameObject();
	gameObj.color = color;
	gameObj.transform.Scale.x = radius;
	gameObj.pointLight = std::make_unique<PointLightComponent>();
	gameObj.pointLight->lightIntensity = intensity;
	return gameObj;
}