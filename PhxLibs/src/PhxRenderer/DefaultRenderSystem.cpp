#include "PhxRenderer/PhxRenderer_pch.h"
#include "DefaultRenderSystem.h"

#include "MeshResource.h"

#include <PhxWorld/Entity.h>
#include <PhxWorld/World.h>
#include <PhxWorld/WorldComponents.h>

#include <PhxEngine/Memory/FrameMemoryManager.h>

#include <PhxRenderer/RenderComponents.h>

#include <PhxResource/IResource.h>

#define ENABLE_ENTT_CALLBACKS false

using namespace phx;

namespace
{
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
	for (auto layer : m_layers)
	{
		if (layer)
			delete layer;
	}

	m_layers.clear();
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

void phx::gfx::DefaultRenderSystem::PreRender(World& world)
{
	PHX_PROFILE;
	// TODO: Determine where this should go
	for (entt::entity entityId : m_observer)
	{
		Entity entity = { entityId, &world };
		if (entity.HasComponent<RenderMeshComponent>())
			continue;

		auto& meshComp = entity.GetComponent<MeshComponent>();
		PHX_CORE_INFO("Mesh Component was added {0}", meshComp.Mesh.c_str());
		RefCountPtr<IResource> resource = nullptr; //ResourceManger::Get(meshComp.Mesh.c_str());
		if (resource)
		{
			auto& renderComponent = entity.AddComponent<RenderMeshComponent>();
			renderComponent.MeshResource = resource;
		}
	}

	m_observer.clear();

	// Construct Views
	m_perFrameCache = {};
	CacheRenderViews(world);
	CacheLayerData(world);
}

void phx::gfx::DefaultRenderSystem::Render(RenderPass /*renderPass*/)
{
	PHX_PROFILE;
#if false
	for (auto layer : m_layers)
	{
		// TODO: Cache data
		layer->Render(renderPass, nullptr);

	}

	size_t offset = static_cast<size_t>(renderPass) * static_cast<size_t>(RenderPass::Count) * m_perFrameCache.NumViews * m_layers.size();

	void** cacheArray = m_perFrameCache.CachedData + offset;
	// TODO: Thread this
	for (size_t iView = 0; iView < m_perFrameCache.NumViews; iView++)
	{
		for (size_t iLayer = 0; iLayer < m_layers.size(); iLayer++)
		{
			// Is there a performance hit here with virtals?
			const size_t index = iView * m_perFrameCache.NumViews * static_cast<size_t>(RenderPass::Count) + iLayer;
		}
	}
#endif
}

void phx::gfx::DefaultRenderSystem::AddLayer(RenderLayer* layer)
{
	m_layers.push_back(layer);
}

void phx::gfx::DefaultRenderSystem::CacheRenderViews(World& world)
{
	PHX_PROFILE;

	m_perFrameCache.Views = phx_new_frame(View);

	auto camerasView = world.GetAllEntitiesWith<phx::NameComponent, phx::CameraComponent>();
	for (auto e : camerasView)
	{
		auto [nameComp, cameraComp] = camerasView.get<phx::NameComponent, phx::CameraComponent>(e);

		if (cameraComp.Active)
		{
			const float nearZ = cameraComp.ZNear;
			const float farZ = cameraComp.ZFar;

			auto viewMatrix = DirectX::XMMatrixLookToRH(
				DirectX::XMLoadFloat3(&cameraComp.Eye),
				DirectX::XMLoadFloat3(&cameraComp.Forward),
				DirectX::XMLoadFloat3(&cameraComp.Up));
			// auto viewMatrix = this->ConstructViewMatrixLH();

			auto* view = new (m_perFrameCache.Views) View();
			
			DirectX::XMStoreFloat4x4(&view->ViewMatrix, viewMatrix);
			DirectX::XMStoreFloat4x4(&view->InvViewMatrix, DirectX::XMMatrixInverse(nullptr, viewMatrix));

			float aspectRatio = cameraComp.Width / cameraComp.Height;
			auto projectionMatrix = DirectX::XMMatrixPerspectiveFovRH(cameraComp.FoV, aspectRatio, nearZ, farZ);
			DirectX::XMStoreFloat4x4(&view->ProjectionMatrix, projectionMatrix);
			DirectX::XMStoreFloat4x4(&view->InvProjectionMatrix, DirectX::XMMatrixInverse(nullptr, projectionMatrix));

			// -- VP
			auto viewProjectionMatrix = viewMatrix * projectionMatrix;
			DirectX::XMStoreFloat4x4(&view->WorldToClipMatrix, viewProjectionMatrix);

			auto viewProjectionInv = DirectX::XMMatrixInverse(nullptr, viewProjectionMatrix);
			DirectX::XMStoreFloat4x4(&view->InvWorldToClipMatrix, viewProjectionInv);

#if false
			this->FrustumWS = Core::Frustum(viewProjectionMatrix, false);
			this->FrustumVS = Core::Frustum(projectionMatrix, false);
#endif
		}

	}
}

void phx::gfx::DefaultRenderSystem::CacheLayerData(World& world)
{
	PHX_PROFILE;
	
	//const size_t cacheDataSize =  static_cast<size_t>(RenderPass::Count) * m_perFrameCache.NumViews * m_layers.size();

	m_perFrameCache.CachedData = phx_new_frame void*;

	// TODO: Thread this
	for (size_t iPass = 0; iPass < static_cast<size_t>(RenderPass::Count); iPass++)
	{
		for (size_t iView = 0; iView < m_perFrameCache.NumViews; iView++)
		{
			for (size_t iLayer = 0; iLayer < m_layers.size(); iLayer++)
			{
				// Is there a performance hit here with virtals?
				const size_t index = iView * m_perFrameCache.NumViews + iPass * static_cast<size_t>(RenderPass::Count) + iLayer;
				m_perFrameCache.CachedData[index] =
					m_layers[iLayer]->PreRender(world, m_perFrameCache.Views[iView], RenderPass::Forward);
			}
		}
	}
}
