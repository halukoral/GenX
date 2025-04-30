#pragma once
#include "pch.h"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include "ECS/Components/TransformComponent.h"
#include "Renderer/Model.h"

struct PointLightComponent
{
	float lightIntensity = 1.0f;
};

class GameObject
{
public:
	using id_t = unsigned int;
	using Map = std::unordered_map<id_t, GameObject>;
	
	static GameObject createGameObject() {
		static id_t currentId = 0;
		return GameObject{currentId++};
	}

	GameObject(const GameObject &) = delete;
	GameObject &operator=(const GameObject &) = delete;
	GameObject(GameObject &&) = default;
	GameObject &operator=(GameObject &&) = default;

	id_t getId() { return id; }

	static GameObject makePointLight(
		float intensity = 10.f,
		float radius = 0.1f,
		glm::vec3 color = glm::vec3(1.f)
	);
	
	glm::vec3 color{};
	TransformComponent transform{};

	std::shared_ptr<Model> model{};
	std::unique_ptr<PointLightComponent> pointLight = nullptr;

private:
	GameObject(id_t objId) : id{objId} {}
	id_t id;
};
