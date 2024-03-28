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

        virtual void PreRender(tf::Subflow& subflow) = 0;
        virtual void FixedUpdate(tf::Subflow& subflow) = 0;
        virtual void Update(TimeStep const deltaTime, tf::Subflow& subflow) = 0;
        virtual void Render(tf::Subflow& subflow) = 0;

        virtual ~IApplication() = default;
    };
}