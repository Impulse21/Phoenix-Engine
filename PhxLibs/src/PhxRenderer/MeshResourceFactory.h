#pragma once

#include <PhxResource/IResource.h>
#include <PhxResource/IResourceFactory.h>

namespace phx::renderer
{
	class MeshResourceFactory : public phx::IResourceFactory
	{
	public:
		RefCountPtr<IResource> Create(StringHash filename, const char* name) override;
	};
}

namespace phx
{
	template<>
	struct ResourceFactoryExtension<renderer::MeshResourceFactory>
	{
		static constexpr const char* value = ".phxmsh";
	};
}

