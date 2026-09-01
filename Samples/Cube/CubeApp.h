#pragma once

#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/RHI/RHITypes.h>

#include <PhxEngine/IApplication.h>

namespace samples
{
    class CubeApp final : public phx::IApplication
    {
    public:
        CubeApp() = default;
        ~CubeApp() override = default;

    public:
        const char* GetName() const override;
    
        // -- Application interface impl ---
    public:
        void OnInit() override;
        void OnPreRender() override;
        void OnUpdate(float dt) override;
        void OnRender(const phx::FrameRenderTargets& targets) override;
        void OnShutdown() override;


    private:
        phx::rhi::CommandBufferHandle m_command_buffer;

        phx::MemoryBuffer m_vertex_shader_spirv;
        phx::MemoryBuffer m_fragment_shader_spirv;
    };
}