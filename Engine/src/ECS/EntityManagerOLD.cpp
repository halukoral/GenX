#include "EntityManagerOLD.h"

#include "Entity.h"

Ref<EntityManagerOLD> EntityManagerOLD::s_Instance = CreateRef<EntityManagerOLD>();

void EntityManagerOLD::Clear() const
{
	for (const auto& entity : m_Entities)
	{
		entity->ShutDown();
	}
}

Ref<Entity> EntityManagerOLD::FindEntityByName(const std::string& entityName) const
{
	for (auto entity : m_Entities)
	{
		if (entity->GetName() == entityName)
		{
			return entity;
		}
	}
	return nullptr;
}

Ref<Entity> EntityManagerOLD::GetEntity(const uint32_t id) const
{
	for (auto entity : m_Entities)
	{
		if (entity->GetID() == id)
		{
			return entity;
		}
	}
	return nullptr;
}

void EntityManagerOLD::RemoveEntity(uint32_t entityId)
{
	const auto it = std::ranges::find_if(m_Entities,
		[entityId](const Ref<Entity>& entity) -> bool { return entity->GetID() == entityId; }
	);

	if (it != m_Entities.end())
	{
		m_Entities.erase(it);
	}
}
