#include "HordeApp.h"

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/Log.h>
#include <PhxEngine/RHI/RHI.h>
#include <PhxEngine/VFS/VFS.h>

#include <PhxEngine/Platform/EntryPoint.h>
#include <PhxEngine/Engine.h>

#include <PhxEngine/ECS/EntityId.h>
#include <PhxEngine/ECS/LinearStorage.h>
#include <PhxEngine/ECS/SparseSet.h>

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

    PHX_LOG_INFO(Log::Channels::App, "Running storage smoke tests");
    {
        const ecs::EntityId slot2_gen0 = ecs::MakeEntityId(2, 0);
        const ecs::EntityId slot2_gen1 = ecs::MakeEntityId(2, 1); // same index, recycled

        // -- LinearStorage ---
        ecs::LinearStorage<TransformComponent> linear;

        if (linear.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: LinearStorage.Has() true before Emplace");

        linear.Emplace(slot2_gen0, hlslpp::float3(1.0f, 2.0f, 3.0f));

        if (!linear.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: LinearStorage.Has() false after Emplace");

        linear.Remove(slot2_gen0);

        if (linear.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: LinearStorage.Has() true after Remove");

        // Recycle the same index under a new generation -- the old handle must
        // not read back as present, or read the new entity's data.
        linear.Emplace(slot2_gen1, hlslpp::float3(4.0f, 5.0f, 6.0f));

        if (linear.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: LinearStorage.Has() accepted a stale handle for a recycled index");

        if (TransformComponent* transform = linear.TryGet(slot2_gen1))
        {
            if (transform->position.x != 4.0f)
                PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: LinearStorage recycled slot has wrong data");
        }
        else
        {
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: LinearStorage.TryGet() missing after recycled Emplace");
        }

        // -- SparseSet ---
        ecs::SparseSet<CapsuleRenderComponent> sparse;

        if (sparse.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: SparseSet.Has() true before Emplace");

        sparse.Emplace(slot2_gen0, 0.5f, 2.0f);

        if (!sparse.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: SparseSet.Has() false after Emplace");

        sparse.Remove(slot2_gen0);

        if (sparse.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: SparseSet.Has() true after Remove");

        sparse.Emplace(slot2_gen1, 9.0f, 9.0f);

        if (sparse.Has(slot2_gen0))
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: SparseSet.Has() accepted a stale handle for a recycled index");

        if (CapsuleRenderComponent* capsule = sparse.TryGet(slot2_gen1))
        {
            if (capsule->radius != 9.0f)
                PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: SparseSet recycled slot has wrong data");
        }
        else
        {
            PHX_LOG_ERROR(Log::Channels::App, "Storage test failed: SparseSet.TryGet() missing after recycled Emplace");
        }
    }
    PHX_LOG_INFO(Log::Channels::App, "Storage smoke tests complete");

    // -- Smoke test the World ---
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

    m_world.FreeEntity(plane_entity);

    // FreeEntity() is required to clean up the freed entity's components --
    // stale entries left behind in the sparse stores would leak memory and,
    // once indices get recycled, could be misread as belonging to the new
    // entity that reused the index. These are expected to pass; if they
    // don't, FreeEntity() still needs component cleanup added.
    if (m_world.TryGet<TransformComponent>(plane_entity))
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: TransformComponent still present after FreeEntity");

    if (m_world.TryGet<PlaneRenderComponent>(plane_entity))
        PHX_LOG_ERROR(Log::Channels::App, "World test failed: PlaneRenderComponent still present after FreeEntity");

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
