#pragma once
#include <ranges>

#include "Entity.h"

// Chunk (her archetype’ın içinde birden fazla olabilir)
struct Chunk
{
	std::vector<Entity2> entities;
	size_t Entity2Count = 0;

	// Komponentlerin ham verisi burada tutulur
	std::unordered_map<ComponentTypeId, std::vector<uint8_t>> data;

	template<typename... Components>
	size_t AddEntity2(const Entity2& Entity2, Components&&... comps)
	{
		size_t idx = Entity2Count++;
		entities.push_back(Entity2);
		(AddComponent(idx, std::forward<Components>(comps)), ...);
		return idx;
	}

	template<typename T>
	void AddComponent(size_t idx, T&& component)
	{
		auto& vec = data[GetComponentTypeId<T>()];
		if (vec.size() < (idx + 1) * sizeof(T)) vec.resize((idx + 1) * sizeof(T));
			memcpy(&vec[idx * sizeof(T)], &component, sizeof(T));
	}

	template<typename T>
	T& GetComponent(size_t idx)
	{
		auto& vec = data[GetComponentTypeId<T>()];
		return *reinterpret_cast<T*>(&vec[idx * sizeof(T)]);
	}
};

// Archetype
struct Archetype
{
	Archetype(std::vector<ComponentTypeId> t) : types(std::move(t))
	{
		chunks.push_back(std::make_unique<Chunk>());
	}

	template<typename... Components>
	bool HasComponents() const
	{
		const std::vector<ComponentTypeId> query = { GetComponentTypeId<Components>()... };
		for (const auto& q : query)
		{
			if (std::ranges::find(types, q) == types.end()) return false;
		}
		return true;
	}

	std::vector<ComponentTypeId> types;
	std::vector<std::unique_ptr<Chunk>> chunks;
};


class ECS
{
public:
	ECS() : nextEntity2Id(1) {}

	template<typename... Components>
	Entity2 CreateEntity2(Components&&... comps)
	{
		auto archetypeId = GetOrCreateArchetype<Components...>();
		auto& archetype = archetypes[archetypeId];
		Chunk* chunk = GetAvailableChunk(archetype);

		Entity2 Entity2{ nextEntity2Id++, 1 };
		size_t idxInChunk = chunk->AddEntity2(Entity2, std::forward<Components>(comps)...);
		entities[Entity2.id] = { archetypeId, chunk, idxInChunk, Entity2.version };
		return Entity2;
	}

	template<typename... Components, typename Func>
	void Query(Func func)
	{
		auto archetypeId = ArchetypeSignature<Components...>();
		for (auto& archetype : archetypes | std::views::values)
		{
			if (archetype.HasComponents<Components...>())
			{
				for (auto& chunkPtr : archetype.chunks)
				{
					Chunk* chunk = chunkPtr.get();
					for (size_t i = 0; i < chunk->Entity2Count; ++i)
					{
						Entity2 Entity2 = chunk->entities[i];
						func(Entity2, chunk->GetComponent<Components>(i)...);
					}
				}
			}
		}
	}

	// ...Component ekleme/çıkarma/destroy fonksiyonları eklenebilir

private:
	EntityId2 nextEntity2Id;

	struct Entity2Record
	{
		size_t archetypeId;
		Chunk* chunk;
		size_t indexInChunk;
		uint32_t version;
	};
	std::unordered_map<EntityId2, Entity2Record> entities;
	std::unordered_map<size_t, Archetype> archetypes;

	// Archetype ve Chunk ile ilgili fonksiyonlar
	template<typename... Components>
	size_t ArchetypeSignature()
	{
		std::vector<ComponentTypeId> ids = { GetComponentTypeId<Components>()... };
		std::sort(ids.begin(), ids.end(), [](auto a, auto b) { return a.hash_code() < b.hash_code(); });
		size_t hash = 0;
		for (const auto& id : ids) hash ^= id.hash_code();
		return hash;
	}

	template<typename... Components>
	size_t GetOrCreateArchetype()
	{
		const size_t sig = ArchetypeSignature<Components...>();
		if (!archetypes.contains(sig))
		{
			archetypes[sig] = Archetype({ GetComponentTypeId<Components>()... });
		}
		return sig;
	}

	Chunk* GetAvailableChunk(Archetype& archetype);

};

inline Chunk* ECS::GetAvailableChunk(Archetype& archetype)
{
	// Basitçe ilk chunk’ı kullan, dolarsa yeni ekle
	if (archetype.chunks.empty() || archetype.chunks.back()->Entity2Count >= CHUNK_CAPACITY)
		archetype.chunks.push_back(std::make_unique<Chunk>());
	return archetype.chunks.back().get();
}
