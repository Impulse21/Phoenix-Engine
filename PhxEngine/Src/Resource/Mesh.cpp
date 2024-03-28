#include <PhxEngine/Resource/Mesh.h>

using namespace PhxEngine;

void PhxEngine::MeshResourceFileHandler::RegisterResourceFile(std::string_view file)
{
}

RefCountPtr<Mesh> PhxEngine::MeshResourceRegistry::Retrieve(StringHash id)
{
	return RefCountPtr<Mesh>();
}