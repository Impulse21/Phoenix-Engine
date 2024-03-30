#include <PhxEngine/Resource/Mesh.h>

using namespace PhxEngine;

void PhxEngine::MeshResourceFileHandler::RegisterResourceFile(std::string_view file)
{
    File f = {
        .Filename = std::string(file),
    };

    const auto id = this->m_files.size();
    this->m_files.push_back(std::move(f));
}

RefCountPtr<Mesh> PhxEngine::MeshResourceRegistry::Retrieve(StringHash id)
{
	return RefCountPtr<Mesh>();
}