#pragma once

namespace PhxEngine
{
    class IApplication
    {
    public:
        virtual void Startup() = 0;
        virtual void Shutdown() = 0;

        virtual ~IApplication() = default;
    };
}