#pragma once

#include "EntityId.h"

namespace phx::ecs
{
    enum class StorageKind : u8
    {
        Sparse = 0,
        Linear,
        Singleton,
        NumStorageTypes
    };

    class IStorage
    {
    public:
        virtual ~IStorage() = default;

        virtual StorageKind GetKind() const = 0;
        virtual void Remove(EntityId e) = 0;
        virtual bool Has(EntityId e) const = 0;
    };
}