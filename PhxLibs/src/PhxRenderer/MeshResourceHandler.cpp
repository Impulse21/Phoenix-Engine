#include "PhxRenderer_pch.h"

#include "MeshResourceHandler.h"
#include "MeshResource.h"

#include <PhxResource/ChunkFileFormat.h>

using namespace phx;
using namespace phx::renderer;


RefCountPtr<IResource> MeshResourceHandler::Load(std::filesystem::path const& /*path*/, std::shared_ptr<IResourceFileSystem> const& /*fs*/ ) const
{
	return RefCountPtr<IResource>::Create(new MeshResource());
}

RefCountPtr<IResource> MeshResourceHandler::Load(std::filesystem::path const& path, std::shared_ptr<IResourceFileSystem> const& fs, FileHandle filehandle, size_t offset) const
{
	return RefCountPtr<IResource>::Create(new MeshResource());
}
