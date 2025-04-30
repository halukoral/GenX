#pragma once
#include "Core.h"

enum class ComponentType
{
	Arrow,
	Camera,
	CameraController,
	EditorCameraController,
	Light,
	Material,
	Tag,
	Transform,
	StaticMesh
};

#define COMPONENT_CLASS_TYPE(type)  static ComponentType GetStaticType() { return ComponentType::type; }\
virtual ComponentType GetComponentType() const override { return GetStaticType(); }\
virtual const char* GetName() const override { return #type; }

class Entity;

class Component
{
public:
	
	Component() = default;
	Component(const Ref<Entity>& entity) : m_Entity(entity) {}
	Component(const Component& other) : m_Entity(other.m_Entity) {}
	virtual ~Component() { m_Entity.reset(); }
	
	void SetEntity(Ref<Entity> entity) { m_Entity = entity; }
	Ref<Entity> GetEntity() const { return m_Entity.lock(); }
	
	virtual ComponentType GetComponentType() const = 0;
	virtual const char* GetName() const = 0;
	
protected:
	Weak<Entity> m_Entity;
};
