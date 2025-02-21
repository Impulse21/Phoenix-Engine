#pragma once

#include <PhxResource/IResource.h>
#include <PhxResource/IResourceHandler.h>

namespace phx::renderer
{
	class MeshResourceHandler : public phx::IResourceHandler
	{
	public:
		RefCountPtr<IResource> Load(std::filesystem::path const& path, std::shared_ptr<IResourceFileSystem> const& fs) const override;
		RefCountPtr<IResource> Load(std::filesystem::path const& path, std::shared_ptr<IResourceFileSystem> const& fs, FileHandle filehandle, size_t offset) const override;

	};
}

namespace phx
{
	template<>
	struct ResourceExtension<renderer::MeshResourceHandler>
	{
		static constexpr const char* value = ".phxmsh";
	};
}

