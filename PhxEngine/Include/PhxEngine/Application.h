#pragma once

#include <taskflow/taskflow.hpp>
#include <PhxEngine/Core/TimeStep.h>

namespace PhxEngine
{
    class IApplication
    {
    public:
        virtual void Startup() = 0;
        virtual void Shutdown() = 0;

        virtual void PreRender() = 0;
        virtual void FixedUpdate() = 0;
        virtual void Update(TimeStep const deltaTime) = 0;
        virtual void Render() = 0;

        virtual ~IApplication() = default;
    };
}