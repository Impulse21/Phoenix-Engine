#pragma once

#include "IStorage.h"

#include "EntityId.h"

#include <PhxEngine/Core/Span.h>
#include <vector>

namespace phx::ecs
{
    template<class T>
    class SparseSet : public IStorage
    {
    public:

      StorageKind GetKind() const override { return StorageKind::Sparse; }

      template<typename... Args>
      T&    Emplace(EntityId e, Args&&... args);
      T&    Emplace(EntityId e, T&& component);
      T&    Insert(EntityId e, T value = {});

      void  Remove(EntityId e) override;
      bool  Has(EntityId e) const override;
      T*    TryGet(EntityId e);
      const T* TryGet(EntityId e) const;

      u32   Size() const { return static_cast<u32>(m_dense.size()); }

      T*    begin() { return m_dense.data(); }
      T*    end() { return m_dense.data() + m_dense.size(); }

      const T*  cbegin() const { return m_dense.data(); }
      const T*  cend() const { return m_dense.data() + m_dense.size(); }
      
      Span<EntityId> GetEntities() const { return m_dense_entities; }
      Span<T> GetDenseMap() const { return m_dense; }

    private:
        std::vector<u32> m_sparse_set;
        std::vector<T> m_dense;
        std::vector<EntityId> m_dense_entities;
    };

    // -- Implementations ---

    template<class T>
    inline T& SparseSet<T>::Insert(EntityId e, T value)
    {
        if (e.Index() >= m_sparse_set.size())
            m_sparse_set.resize(e.Index() + 1, EntityId::Null);

        PHX_ASSERT(m_sparse_set[e.Index()] == EntityId::Null && "Entity alreayd has this component");
        
        m_sparse_set[e.Index()] = static_cast<u32>(m_dense.size());
        m_dense.emplace_back(std::move(value));
        m_dense_entities.push_back(e);
        return m_dense.back();
    }

    template<class T>
    inline void SparseSet<T>::Remove(EntityId e)
    {
        PHX_ASSERT(Has(e) && "No found in ecs");

        const u32 dense_index = m_sparse_set[e.Index()];

        // Swap and pop;
        std::swap(m_dense[dense_index], m_dense.back());
        std::swap(m_dense_entities[dense_index], m_dense_entities.back());

        m_dense.pop_back();
        m_dense_entities.pop_back();
        m_sparse_set[e.Index()] = EntityId::Null;
    }

    template<class T>
    inline T* SparseSet<T>::TryGet(EntityId e)
    {
        if (!Has(e))
            return nullptr;

        const u32 dense_index = m_sparse_set[e.Index()];

        return &m_dense[dense_index];
    }

    template<class T>
    inline const T* SparseSet<T>::TryGet(EntityId e) const
    {
        if (!Has(e))
            return nullptr;

        const u32 dense_index = m_sparse_set[e.Index()];

        return &m_dense[dense_index];
    }

    template<class T>
    inline bool SparseSet<T>::Has(EntityId e) const
    {
        if (e.Index() >= m_sparse_set.size())
            return false;

        const u32 dense_index = m_sparse_set[e.Index()];
        if (dense_index == EntityId::Null)
            return false;
            
        return m_dense_entities[dense_index].value == e.value;
    }

    template<class T>
    template<typename... Args>
    inline T& SparseSet<T>::Emplace(EntityId e, Args&&... args)
    {
        if (e.Index() >= m_sparse_set.size())
            m_sparse_set.resize(e.Index() + 1, EntityId::Null);

        PHX_ASSERT(m_sparse_set[e.Index()] == EntityId::Null && "Entity alreayd has this component");
        
        m_sparse_set[e.Index()] = static_cast<u32>(m_dense.size());
        m_dense.emplace_back(std::forward<Args>(args)...);
        m_dense_entities.push_back(e);
        return m_dense.back();
    }

    template<class T>
    inline T& SparseSet<T>::Emplace(EntityId e, T&& component)
    {
        if (e.Index() >= m_sparse_set.size())
            m_sparse_set.resize(e.Index() + 1, EntityId::Null);

        PHX_ASSERT(m_sparse_set[e.Index()] == EntityId::Null && "Entity alreayd has this component");
        
        m_sparse_set[e.Index()] = static_cast<u32>(m_dense.size());
        m_dense.emplace_back(std::forward<T>(component));
        m_dense_entities.push_back(e);
        return m_dense.back();
    }

}  // namespace phx::ecs