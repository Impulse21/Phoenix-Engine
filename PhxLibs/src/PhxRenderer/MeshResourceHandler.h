#pragma once

#include <PhxResource/IResource.h>
#include <PhxResource/IResourceHandler.h>

namespace phx::renderer
{
	class MeshResourceHandler : public phx::IResourceHandler
	{
	public:
		RefCountPtr<IResource> Load(StringHash filename);
	};
}

namespace phx
{
	template<>
	struct ResourceHandlerExtension<renderer::MeshResourceHandler>
	{
		static constexpr const char* value = ".phxmsh";
	};
}

