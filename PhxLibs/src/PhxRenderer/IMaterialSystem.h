#pragma once

#include <PhxCore/RefCountPtr.h>

namespace phx::renderer
{
	struct MaterialResource : public RefCounted
	{
	};

	struct MaterialInstance : public RefCounted
	{
	};

	struct MaterialArchetype : public RefCounted 
	{
	};

	class MaterialSystem
	{
	public:
		~MaterialSystem() = default;

		virtual void Initialize(const char* archetypes_root_path) = 0;
		virtual void Shutdonw() = 0;

		virtual RefCountPtr<MaterialInstance> MaterialSystem::CreateFromResource(RefCounter<MaterialResource> resource) = 0;
	};
}