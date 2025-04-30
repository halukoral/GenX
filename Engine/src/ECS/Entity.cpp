#include "Entity.h"

#include "spdlog/spdlog.h"

static uint32_t EntityId = 1;

Entity::Entity(const Entity& other)
{
	m_Id = other.m_Id;
	m_Name = other.m_Name;
	m_IdForMousePick = other.m_IdForMousePick;
}

Entity::Entity(const std::string& name)
{
	m_Id = ++EntityId;
	m_Name = name;
	m_IdForMousePick = ++EntityId;
}

Entity::Entity(const uint32_t& uuid, const std::string& name)
{
	m_Id = uuid;
	m_Name = name;
	m_IdForMousePick = ++EntityId;
}

void Entity::ShutDown()
{
	for (auto cmp : m_Components)
	{
		cmp.reset();
	}
	m_Components.clear();
	m_ComponentTypeMap.clear();
	//spdlog::warn("Entity {0} {1} destroyed!", m_Id, m_Name);
}
