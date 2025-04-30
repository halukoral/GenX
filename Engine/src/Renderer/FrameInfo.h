#pragma once

// lib
#include <glm/fwd.hpp>
#include <glm/vec4.hpp>
#include <vulkan/vulkan.h>

#include "Actor/CameraActor.h"
#include "Actor/GameObject.h"

#define MAX_LIGHTS 10

struct PointLight
{
	glm::vec4 Position{};
	glm::vec4 Color{};
};

struct GlobalUbo
{
	glm::mat4 Projection{1.f};
	glm::mat4 View{1.f};
	glm::mat4 InverseView{1.f};
	glm::vec4 AmbientLightColor{1.f, 1.f, 1.f, .02f};
	
	PointLight PointLights[MAX_LIGHTS];
	int NumLights;
};

struct FrameInfo
{
	int FrameIndex;
	float FrameTime;
	VkCommandBuffer CommandBuffer;
	CameraActor& Camera;
	VkDescriptorSet GlobalDescriptorSet;
	GameObject::Map& GameObjects;
};