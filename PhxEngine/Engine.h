#pragma once

#include <PhxEngine/Memory/Memory.h>
#include <PhxEngine/Core/Span.h>
#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Memory/FrameAllocator.h>

namespace phx
{
    class IApplication;
    
    struct FrameData
    {
        FrameAllocator* allocator;

        float delta;
        u32 frame_index;
        Span<renderer::RenderView> views;
    };

    namespace Engine
    {
        void Initialize(IApplication* app, Span<char*> args);
        void Run();
        void Shutdown();

        void RequestExit();
        
    }
}