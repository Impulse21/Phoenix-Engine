#pragma once

#include <PhxCore/Handle.h>
#include <PhxResource/ResourcePtr.h>

namespace phx
{
	namespace renderer
	{
		struct MeshResource;
		struct MaterialResource;
		struct TextureResource;
	}

	using MeshResourceHandle = Handle<renderer::MeshResource>;
	using MeshResourcePtr = ResourcePtr<renderer::MaterialResource>;

	using MaterialResourceHandle = Handle<renderer::MaterialResource>;
	using MaterialResourcePtr = ResourcePtr<renderer::MaterialResource>;

	using TextureResourceHandle = Handle<renderer::TextureResource>;
	using TextureResourcePtr = ResourcePtr<renderer::TextureResource>;
}