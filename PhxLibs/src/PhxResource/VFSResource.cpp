#include "PhxResource_pch.h"

#include "VFSResource.h"

using namespace phx;

std::unique_ptr<IResourceFileSystem> phx::FileSystemFactory::CreateResourceFileSystem()
{

	return std::unique_ptr<IResourceFileSystem>();
}
