#include "PhxRenderer/PhxRenderer_pch.h"
#include "DefaultRenderSystem.h"

#include <PhxWorld/Entity.h>
#include <PhxWorld/World.h>
#include <PhxWorld/WorldComponents.h>

#include <PhxResource/IResource.h>
#include <PhxResource/ResourceManger.h>

#define ENABLE_ENTT_CALLBACKS false

using namespace phx;

namespace
{
	struct RenderMeshComponent
	{
		RefCountPtr<IResource> MeshResource;
	};

#if ENABLE_ENTT_CALLBACKS
    namespace WorldCallbacks
    {
        void OnConstruct(entt::registry&, entt::entity)
        {
        }
    }
#endif
}
void phx::gfx::DefaultRenderSystem::Initialize()
{
}

void phx::gfx::DefaultRenderSystem::Finalize()
{
}

void phx::gfx::DefaultRenderSystem::RegisterObservers(phx::World& world)
{
#if ENABLE_ENTT_CALLBACKS
    world.GetRegistry().on_construct<MeshComponent>().connect<&WorldCallbacks::OnConstruct>();

#else
    // This observer tracks when MeshComponent is constructed on any entity
    m_observer.connect(
        world.GetRegistry(),
        entt::collector
        .group<MeshComponent>());  // group just ensures it's on construct
#endif
}

void phx::gfx::DefaultRenderSystem::OnPreRender(World& world)
{
	// TODO: Determine where this should go
	for (entt::entity entityId : m_observer)
	{
		Entity entity = { entityId, &world };
		if (entity.HasComponent<RenderMeshComponent>())
			continue;

		auto& meshComp = entity.GetComponent<MeshComponent>();
		PHX_CORE_INFO("Mesh Component was added {0}", meshComp.Mesh.c_str());
		RefCountPtr<IResource> resource = ResourceManger::Get(meshComp.Mesh.c_str());
		if (resource)
		{
			auto& renderComponent = entity.AddComponent<RenderMeshComponent>();
			renderComponent.MeshResource = resource;
		}
	}

	m_observer.clear();
}

void phx::gfx::DefaultRenderSystem::OnRender(rhi::CommandCtx* /*ctx*/, void* /*cachedData*/)
{
}

void phx::gfx::DefaultRenderSystem::RegisterSubSystem(uint32_t /*passMask*/, std::shared_ptr<IRenderSubSystem> /*subSystem*/)
{
}
