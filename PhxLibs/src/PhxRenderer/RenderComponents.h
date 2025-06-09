#pragma once

namespace phx
{
	struct Resource;
}

namespace phx::gfx
{
	struct RenderMeshComponent
	{
		RefCountPtr<Resource> MeshResource;
	};
}