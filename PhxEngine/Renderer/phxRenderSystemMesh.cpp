#include "pch.h"
#include "phxRenderSystem.h"
#include "World/phxWorld.h"

#include "Core/phxMemory.h"
#include "RHI/PhxRHI.h"

using namespace phx;
using namespace DirectX;

namespace
{
	rhi::BufferHandle gVB_Cube;
	rhi::BufferHandle gIB_Cube;

	struct MeshDrawable
	{
		rhi::BufferHandle VertexBuffer;
		rhi::BufferHandle IndexBuffer;
	};
}

void* phx::RenderSystemMesh::CacheData(PHX_UNUSED World const& world)
{
	// const entt::registry& registry = world.GetRegistry();

	//auto& allocator = Memory::GetFrameAllocator();

	
	return nullptr;
}

void phx::RenderSystemMesh::Render()
{
}