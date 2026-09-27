#pragma once

#include "EntityId.h"

namespace phx::ecs
{
    class IStorage
    {
    public:
        virtual ~IStorage() = default;

        virtual void Remove(EntityId e) = 0;
        virtual bool Has(EntityId e) = 0;
    };
}