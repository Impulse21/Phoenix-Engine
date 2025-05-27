#pragma once

namespace phx
{
	class IResource;
}

namespace phx::gfx
{
	struct RenderMeshComponent
	{
		RefCountPtr<IResource> MeshResource;
	};
}