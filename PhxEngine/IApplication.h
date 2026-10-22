#pragma once

#include <PhxEngine/RHI/RHITypes.h>
#include <PhxEngine/Renderer/HdrRenderTargets.h>
#include <PhxEngine/Core/Jobs.h>

namespace phx
{
    struct EngineDesc;
    struct FrameData;

    class IApplication
    {
    public:
        virtual ~IApplication() = default;

        virtual const char*             GetName() const                                     = 0;
        virtual void                    OnInit()                                            = 0;

        virtual void OnBuildPreRenderFrame(Jobs::Graph& graph, FrameData* frame_data)       = 0;
        virtual void OnBuildUpdateFrame(Jobs::Graph& graph, const FrameData* frame_data)    = 0;
        virtual void OnBuildRenderFrame(Jobs::Graph& graph, const FrameData* frame_data)    = 0;

        virtual void                OnShutdown()                                            = 0;
    };
}
