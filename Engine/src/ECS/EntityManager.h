#pragma once
#include "Types.h"
#include <array>
#include <queue>
#include <cassert>
#include <unordered_map>
#include <string>

class EntityManager {
public:
    EntityManager() {
        // Entity ID havuzunu oluştur
        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) {
            m_AvailableEntities.push(entity);
        }
    }

    EntityID CreateEntity() {
        assert(m_LivingEntityCount < MAX_ENTITIES && "Too many entities in existence.");

        // Kullanılabilir bir entity ID al
        EntityID id = m_AvailableEntities.front();
        m_AvailableEntities.pop();
        ++m_LivingEntityCount;

        // Yeni entity için boş bir imza oluştur
        m_Signatures[id].reset();
        
        return id;
    }

    void DestroyEntity(EntityID entity) {
        assert(entity < MAX_ENTITIES && "Entity out of range.");

        // Entity imzasını sıfırla ve ID'yi havuza geri ekle
        m_Signatures[entity].reset();
        m_AvailableEntities.push(entity);
        --m_LivingEntityCount;
        
        // Eğer bu entity'nin bir adı varsa, onu da kaldır
        auto it = m_EntityNames.find(entity);
        if (it != m_EntityNames.end()) {
            m_EntityNames.erase(it);
        }
    }

    void SetSignature(EntityID entity, Signature signature) {
        assert(entity < MAX_ENTITIES && "Entity out of range.");

        m_Signatures[entity] = signature;
    }

    Signature GetSignature(EntityID entity) const {
        assert(entity < MAX_ENTITIES && "Entity out of range.");

        return m_Signatures[entity];
    }
    
    // Entity'ye isim atama
    void SetEntityName(EntityID entity, const std::string& name) {
        m_EntityNames[entity] = name;
        m_NameToEntity[name] = entity;
    }
    
    // İsme göre entity bulma
    EntityID FindEntityByName(const std::string& name) const {
        auto it = m_NameToEntity.find(name);
        if (it != m_NameToEntity.end()) {
            return it->second;
        }
        return MAX_ENTITIES; // Geçersiz entity ID
    }
    
    // Entity'nin adını alma
    std::string GetEntityName(EntityID entity) const {
        auto it = m_EntityNames.find(entity);
        if (it != m_EntityNames.end()) {
            return it->second;
        }
        return ""; // İsimsiz entity
    }
    
    size_t GetEntityCount() const { return m_LivingEntityCount; }

private:
    // Entity imzalarını tutan dizi
    std::array<Signature, MAX_ENTITIES> m_Signatures{};
    
    // Kullanılabilir entity ID'lerini tutan kuyruk
    std::queue<EntityID> m_AvailableEntities{};
    
    // Şu anda aktif olan entity sayısı
    size_t m_LivingEntityCount = 0;
    
    // Entity isimleri
    std::unordered_map<EntityID, std::string> m_EntityNames;
    std::unordered_map<std::string, EntityID> m_NameToEntity;
};
