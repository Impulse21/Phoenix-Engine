#pragma once

#include "EntityId.h"
#include <vector>

namespace phx::ecs
{
    template<class T>
    class SparseSetStorage
    {
    public:
      
      T&    Insert(EntityId e, T value = {});
      void  Remove(EntityId e);
      T*    TryGet(EntityId e);
      bool  Has(EntityId e);

      u32   Size() const { return static_cast<u32>(m_dense.size()); }

      T*    begin() { return m_dense.data(); }
      T*    end() { return m_dense.data() + m_dense.size(); }

      const T*  cbegin() const { return m_dense.data(); }
      const T*  cend() const { return m_dense.data() + m_dense.size(); }
      
    private:
        std::vector<u32> m_sparse_set;
        std::vector<T> m_dense;
        std::vector<EntityId> m_dense_entities;
    };

    // -- Implementations ---

    template<class T>
    inline T& SparseSetStorage<T>::Insert(EntityId e, T value)
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
    inline void SparseSetStorage<T>::Remove(EntityId e)
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
    inline T* SparseSetStorage<T>::TryGet(EntityId e)
    {
        if (!Has(e))
            return nullptr;
        
        const u32 dense_index = m_sparse_set[e.Index()];

        return &m_dense[dense_index];
    }

    template<class T>
    inline bool SparseSetStorage<T>::Has(EntityId e)
    {
        return e.Index() < m_sparse_set.size() && m_sparse_set[e.Index()] != EntityId::Null;
    }

}  // namespace phx::ecs