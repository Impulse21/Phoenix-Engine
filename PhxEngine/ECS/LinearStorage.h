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

      template<typename... Args>
      T&    Emplace(EntityId e, Args&&... args);
      T&    Insert(EntityId e, T value = {});
      
      void  Remove(EntityId e) override;
      bool  Has(EntityId e) override;
      T*    TryGet(EntityId e);

      u32   Size() const { return static_cast<u32>(m_dense.size()); }

      T*    begin() { return m_dense.data(); }
      T*    end() { return m_dense.data() + m_dense.size(); }

      const T*  cbegin() const { return m_dense.data(); }
      const T*  cend() const { return m_dense.data() + m_dense.size(); }
      
    private:
        std::vector<T> m_data;
    };

    // -- Implementations ---

    template<class T>
    inline T& LinearStorage<T>::Insert(EntityId e, T value)
    {
        if (e.Index() >= m_data.size())
            m_data.resize(e.Index() + 1, {});

        m_data[e,Index()] = std::move(value);
        return m_data[e,Index()];
    }

    template<class T>
    inline void LinearStorage<T>::Remove(EntityId e)
    {
        PHX_ASSERT(Has(e) && "No found in ecs");
        m_data[e.Index()] = {};
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
        return e.Index() < m_data.size();
    }

    template<class T>
    template<typename... Args>
    inline T& LinearStorage<T>::Emplace(EntityId e, Args&&... args)
    {
        if (e.Index() >= m_data.size())
            m_data.resize(e.Index() + 1, EntityId::Null);

        m_data[e.Index()] = T(std::forward<Args>(args)...);
        return m_data[e.Index()];
    }

}  // namespace phx::ecs