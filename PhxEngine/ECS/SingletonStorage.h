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
        T& Emplace(Args&&... args)
        {
            return m_storage.emplace(std::forward<Args>(args)...);
        }

        T& Set(T value = {})
        {
            m_storage = std::move(value);
            return *m_storage;
        }

        T& Get()
        {
            PHX_ASSERT(m_storage.has_value() && "Singleton has not been set");
            return *m_storage;
        }

        bool Has() const { return m_storage.has_value(); }

        void Clear() { m_storage.reset(); }

    private:
        std::optional<T> m_storage;
    };
}  // namespace phx::ecs