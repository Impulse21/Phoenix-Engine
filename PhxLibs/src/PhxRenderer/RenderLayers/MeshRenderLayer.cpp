#include "PhxRenderer/PhxRenderer_pch.h"
#include "MeshRenderLayer.h"

#include <PhxWorld/World.h>
#include <PhxRenderer/RenderSystem.h>
#include <PhxRenderer/RenderComponents.h>

#include <DirectXMath.h>

#include <PhxRenderer/MeshResource.h>

using namespace phx;
using namespace phx::gfx;

namespace
{
	struct CacheEntry
	{
		RefCountPtr<renderer::MeshResource> Resource;
		DirectX::XMFLOAT4X4 ModelToClipSpace;
	};

	struct MeshRenderLayerCache
	{
		uint32_t NumEntries;
		CacheEntry* Entries;
	};
}

void* phx::gfx::MeshRenderLayer::PreRender(phx::World& world, View const& view, RenderPass renderPass)
{
	if (renderPass != RenderPass::Forward)
		return;

	PHX_PROFILE;

	MeshRenderLayerCache* cache = phx::Memory::GetFrameAllocator().Alloc<MeshRenderLayerCache>();

	// Collect Meshes
	auto componentView = world.GetAllEntitiesWith<TransformComponent, gfx::RenderMeshComponent>();
	size_t estimatedSize = 0;
	for (auto e : componentView)
	{
		estimatedSize++;

	}

	cache->Entries = phx::Memory::GetFrameAllocator().AllocArray<CacheEntry>(estimatedSize);

	cache->NumEntries = 0;
	for (auto e : componentView)
	{
		auto [transformComp, renderMeshComp] = componentView.get<TransformComponent, gfx::RenderMeshComponent>(e);

		CacheEntry& entry = cache->Entries[cache->NumEntries];

		entry.Resource = renderMeshComp.MeshResource.As<renderer::MeshResource>();

		DirectX::XMMATRIX model = DirectX::XMLoadFloat4x4(&transformComp.WorldMatrix);
		DirectX::XMMATRIX worldToClip = DirectX::XMLoadFloat4x4(&view.WorldToClipMatrix);

		DirectX::XMMATRIX modelToClip = model * worldToClip;

		DirectX::XMStoreFloat4x4(&entry.ModelToClipSpace, modelToClip);

		cache->NumEntries++;
	}

	return cache;
}

void phx::gfx::MeshRenderLayer::Render(RenderPass /*renderPass*/, void* /*cachedData*/)
{
}

void phx::gfx::MeshRenderLayer::Finalize()
{
}
