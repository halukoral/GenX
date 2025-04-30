#pragma once
#include "pch.h"
#include "ECS/Component.h"

class TagComponent : public Component
{
public:
	COMPONENT_CLASS_TYPE(Tag)

	std::string Tag;

	TagComponent() = default;
	TagComponent(const TagComponent& other) : Component(other), Tag(other.Tag) {}
	TagComponent(const std::string& tag) : Tag(tag) {}

	TagComponent& operator=(const TagComponent& other)
	{
		m_Entity = other.m_Entity;

		Tag = other.Tag;
		return *this;
	}

};
