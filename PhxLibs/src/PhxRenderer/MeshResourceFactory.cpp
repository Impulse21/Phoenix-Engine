#include "PhxRenderer_pch.h"

#include "MeshResourceFactory.h"
#include "MeshResource.h"

using namespace phx;
using namespace phx::renderer;


RefCountPtr<IResource> phx::renderer::MeshResourceFactory::Create(StringHash, const char*)
{
	return RefCountPtr<IResource>::Create(new MeshResource());
}
