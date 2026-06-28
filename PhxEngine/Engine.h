#pragma once

#include <PhxEngine/Memory/Memory.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/RHI/RHITypes.h>

namespace phx
{
    class IApplication;
    
    namespace Engine
    {
        void Initialize(IApplication* app, Span<char*> args);
        void Run();
        void Shutdown();

        void RequestExit();

        phx::rhi::ViewportHandle GetViewport();
    }
}