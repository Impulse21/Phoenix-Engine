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

	struct PrefabResource;
	using PrefabResourceHandle = Handle<PrefabResource>;
	using PrefabResourcePtr = ResourcePtr<PrefabResource>;

	using MeshResourceHandle = Handle<renderer::MeshResource>;
	using MeshResourcePtr = ResourcePtr<renderer::MeshResource>;

	using MaterialResourceHandle = Handle<renderer::MaterialResource>;
	using MaterialResourcePtr = ResourcePtr<renderer::MaterialResource>;

	using TextureResourceHandle = Handle<renderer::TextureResource>;
	using TextureResourcePtr = ResourcePtr<renderer::TextureResource>;
}