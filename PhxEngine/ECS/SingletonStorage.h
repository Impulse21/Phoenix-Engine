#pragma once

#include <PhxEngine/Core/PhxDefines.h>

#include "IStorage.h"

#include <optional>
#include <utility>

namespace phx::ecs
{
    template<class T>
    class SingletonStorage : public IStorage
    {
    public:

        template<typename... Args>
        T&    Emplace(EntityId e, Args&&... args);
        T&    Insert(EntityId e, T value = {});
      
        void  Remove(EntityId e) override;
        bool  Has(EntityId e) override { return PHX_ASSERT(e == m_entity); m_storage.has_value(); }
        T*    TryGet(EntityId e);

        u32   Size() const { return 1; }

        T*    begin() { return m_dense.data(); }
        T*    end() { return m_dense.data() + m_dense.size(); }

        const T*  cbegin() const { return m_dense.data(); }
        const T*  cend() const { return m_dense.data() + m_dense.size(); }


    private:
        std::optional<T> m_storage;
        EntityId m_entity;
    };
}  // namespace phx::ecs