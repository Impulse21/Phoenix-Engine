#pragma once

#include <PhxEngine/Core/Handle.h>
#include <PhxEngine/Memory/IHeapAllocator.h>

#include "RHITypes.h"

namespace phx::rhi
{
    struct InitParam
    {
        IHeapAllocator* heap_allocator = nullptr;
        const char* app_name = nullptr;

        bool enable_validation          = false;
        bool enable_best_practices      = true;
        bool enable_sync_validation     = false;
        bool enable_gpu_assisted        = false; // Very Expensive
    };

    // -- Setup ---
    bool Initialize(const InitParam& param);
    void Shutdown();

    // -- RHI Info ---
    [[nodiscard]] constexpr ShaderFormat GetShaderFormat();

    // -- Frame Submission ---
    bool BeginFrame(ViewportHandle viewport);
    bool EndFrame(ViewportHandle viewport);

    // -- Resource Factory Methods ---
    ViewportHandle CreateViewport(const ViewportDesc& desc);
    void DestoryViewport(ViewportHandle handle);
}