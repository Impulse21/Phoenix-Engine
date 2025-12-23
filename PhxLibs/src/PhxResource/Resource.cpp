#include "PhxResource/PhxResource_pch.h"

#include "Resource.h"
#include "ResourceManager.h"

using namespace phx;


void phx::Resource::Release() const
{
    if (ref_count.fetch_sub(1) == 1)
    {
        ResourceManager::Destroy(this);
    }
}
