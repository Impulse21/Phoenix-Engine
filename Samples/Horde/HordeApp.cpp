#include "HordeApp.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

#include "WorldComponents.h"

using namespace samples;
using namespace phx;
using namespace horde;


PHX_DEFINE_APP(HordeApp);

const char* samples::HordeApp::GetName() const { return "PhxHorde"; }

void samples::HordeApp::OnInit()
{
    // -- Set up mount mounts ---
    VFS::Mount("shaders://", PHX_SHADER_SOURCE_DIR);
    VFS::Mount("assets://", PHX_ASSET_SOURCE_DIR);

    // -- Smoke test the World ---
    // TODO: FIX DELETEING CLEAN UP AND GENERATION CHECKS
    PHX_LOG_INFO(Log::Channels::App, "Running World smoke tests");

    const ecs::EntityId capsule_entity = m_world.CreateEntity();
    m_world.Emplace<TransformComponent>(capsule_entity, hlslpp::float3(1.0f, 2.0f, 3.0f));
    m_world.Emplace<CapsuleRenderComponent>(capsule_entity, 0.5f, 2.0f);

    if (TransformComponent* transform = m_world.TryGet<TransformComponent>(capsule_entity))
    {
        if (transform->position.x != 1.0f)
            PHX_LOG_ERROR(Log::Channels::App, "World test failed: TransformComponent value mismatch");
    }
    else
    {
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: TransformComponent missing after Emplace");
    }

    if (!m_world.TryGet<CapsuleRenderComponent>(capsule_entity))
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: CapsuleRenderComponent missing after Emplace");

    const ecs::EntityId plane_entity = m_world.CreateEntity();
    m_world.Emplace<TransformComponent>(plane_entity);
    m_world.Emplace<PlaneRenderComponent>(plane_entity, hlslpp::float2(10.0f, 10.0f));

    if (!m_world.TryGet<PlaneRenderComponent>(plane_entity))
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: PlaneRenderComponent missing after Emplace");

    // Singleton component -- shares the Emplace() entry point but is keyed by type, not entity.
    m_world.Emplace<EnvPropertiesComponent>(capsule_entity, true);

    if (EnvPropertiesComponent* env = m_world.TryGet<EnvPropertiesComponent>(capsule_entity))
    {
        if (!env->simple_test_bool)
            PHX_LOG_ERROR(Log::Channels::App, "World test failed: EnvPropertiesComponent value mismatch");
    }
    else
    {
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: EnvPropertiesComponent missing after Emplace");
    }

    // NOTE: FreeEntity() only recycles the entity's index/generation right now --
    // it does not remove the freed entity's components from the sparse stores
    // (World has no per-entity signature to know what to clean up). So this
    // only checks index recycling, not component cleanup -- flagging in case
    // that's not the intended behaviour.
    m_world.FreeEntity(plane_entity);
    const ecs::EntityId recycled_entity = m_world.CreateEntity();

    if (recycled_entity.Index() != plane_entity.Index())
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: freed index was not recycled");

    if (recycled_entity.Generation() != plane_entity.Generation() + 1)
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: recycled entity's generation was not incremented");

    // Capsule entity's components should be untouched by freeing the plane entity.
    if (!m_world.TryGet<CapsuleRenderComponent>(capsule_entity))
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: CapsuleRenderComponent lost after unrelated FreeEntity");

    PHX_LOG_INFO(Log::Channels::App, "World smoke tests complete");

    Engine::RequestExit();
}

void samples::HordeApp::OnBuildPreRenderFrame(phx::Jobs::Graph&)
{
}

void samples::HordeApp::OnBuildUpdateFrame(phx::Jobs::Graph& graph, float dt)
{
    graph.Emplace([this, dt] {
        Update(dt);
    });
}

void samples::HordeApp::OnBuildRenderFrame(phx::Jobs::Graph& graph)
{
    graph.Emplace([this] {
        Render();
    });
}

void samples::HordeApp::Update(float)
{
}

void samples::HordeApp::Render()
{
    rhi::CommandBuffer cmd = rhi::BeginCommandRecording(rhi::CommandQueueType::Graphics);

    rhi::CmdBeginRenderPass({ .colour = { 0.05f, 0.05f, 0.08f, 1.0f } }, cmd);
    rhi::CmdEndRenderPass(cmd);

    rhi::SubmitAndPresent(Span<rhi::CommandBuffer>(&cmd, 1));
}

void samples::HordeApp::OnShutdown()
{
}
