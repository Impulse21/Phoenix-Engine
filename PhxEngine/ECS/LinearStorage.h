#pragma once

#include "IStorage.h"

#include "EntityId.h"
#include <vector>

namespace phx::ecs
{
    template<class T>
    class LinearStorage : public IStorage
    {
    public:

      StorageKind GetKind() const override { return StorageKind::Linear; }

      template<typename... Args>
      T&    Emplace(EntityId e, Args&&... args);
      T&    Insert(EntityId e, T value = {});

      void  Remove(EntityId e) override;
      bool  Has(EntityId e) override;
      T*    TryGet(EntityId e);

      u32   Size() const { return static_cast<u32>(m_data.size()); }

      T*    begin() { return m_data.data(); }
      T*    end() { return m_data.data() + m_data.size(); }

      const T*  cbegin() const { return m_data.data(); }
      const T*  cend() const { return m_data.data() + m_data.size(); }

    private:
        std::vector<T> m_data;
        std::vector<EntityId> m_owner;
    };

    // -- Implementations ---

    template<class T>
    inline T& LinearStorage<T>::Insert(EntityId e, T value)
    {
        if (e.Index() >= m_data.size())
        {
            m_data.resize(e.Index() + 1);
            m_owner.resize(e.Index() + 1);
        }

        m_data[e.Index()] = std::move(value);
        m_owner[e.Index()] = e;
        return m_data[e.Index()];
    }

    template<class T>
    inline void LinearStorage<T>::Remove(EntityId e)
    {
        PHX_ASSERT(Has(e) && "No found in ecs");
        
        m_data[e.Index()] = {};
        m_owner[e.Index()] = EntityId{};
    }

    template<class T>
    inline T* LinearStorage<T>::TryGet(EntityId e)
    {
        if (!Has(e))
            return nullptr;

        return &m_data[e.Index()];
    }

    template<class T>
    inline bool LinearStorage<T>::Has(EntityId e)
    {
        return e.Index() < m_owner.size() && m_owner[e.Index()].value == e.value;
    }

    template<class T>
    template<typename... Args>
    inline T& LinearStorage<T>::Emplace(EntityId e, Args&&... args)
    {
        if (e.Index() >= m_data.size())
        {
            m_data.resize(e.Index() + 1);
            m_owner.resize(e.Index() + 1);
        }

        m_data[e.Index()] = T(std::forward<Args>(args)...);
        m_owner[e.Index()] = e;
        return m_data[e.Index()];
    }

}  // namespace phx::ecs
