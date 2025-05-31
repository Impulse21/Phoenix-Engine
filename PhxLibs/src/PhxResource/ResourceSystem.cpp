#include <PhxResource/PhxResource_pch.h>
#include "ResourceSystem.h"

#include "IResource.h"

using namespace phx;

void phx::ResourceSystem::Initialize()
{
}

void phx::ResourceSystem::Shutdown()
{
}

RefCountPtr<Resource> phx::ResourceSystem::Get(const char* /*path*/)
{
	return nullptr;
}
