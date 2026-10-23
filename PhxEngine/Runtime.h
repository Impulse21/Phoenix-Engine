#pragma once

#include <PhxEngine/Memory/Memory.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Memory/FrameAllocator.h>

#include <PhxEngine/Renderer/Shaders/PhxInterop.h>

namespace phx
{
    class IApplication;
    
    namespace Runtime
    {
        void Initialize(IApplication* app, Span<char*> args);
        void Run();
        void Shutdown();

        void RequestExit();
        
    }
}