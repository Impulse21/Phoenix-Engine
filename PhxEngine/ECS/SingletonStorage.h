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
        StorageKind GetKind() const override { return StorageKind::Singleton; }
        
        template<typename... Args>
        T& Emplace(EntityId, Args&&... args)
        {
            return m_storage.emplace(std::forward<Args>(args)...);
        }

        T& Emplace(EntityId, T&& component)
        {
            return m_storage.emplace(std::forward<T>(component));
        }

        T& Insert(EntityId, T value = {})
        {
            m_storage = std::move(value);
            return *m_storage;
        }

        void Remove(EntityId) override { m_storage.reset(); }
        bool Has(EntityId) const override { return m_storage.has_value(); }

        T* TryGet(EntityId)
        {
            return m_storage.has_value() ? &*m_storage : nullptr;
        }

        const T* TryGet(EntityId) const
        {
            return m_storage.has_value() ? &*m_storage : nullptr;
        }

        u32 Size() const { return m_storage.has_value() ? 1u : 0u; }

    private:
        std::optional<T> m_storage;
    };
}  // namespace phx::ecs