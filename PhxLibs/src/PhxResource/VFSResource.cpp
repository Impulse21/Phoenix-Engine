#include "PhxResource_pch.h"

#include "VFSResource.h"
#include "VFSResource_ds.h"

using namespace phx;

std::unique_ptr<IResourceFileSystem> phx::FileSystemFactory::CreateResourceFileSystem()
{
	return std::make_unique<DSResourceFileSystem>();
}
