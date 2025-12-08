#include "PhxResource/PhxResource_pch.h"
#include "ResourcePrimitives.h"
#include "ResourceManager.h"

void phx::ResourceIncRef(GenericHandle h)
{
	ResourceManager::IncRef(h);
}

void phx::ResourceDecRef(GenericHandle h)
{
	ResourceManager::DecRef(h);
}
