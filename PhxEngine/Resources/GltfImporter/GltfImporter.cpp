#include "GltfImporter.h"

using namespace phx;

RefCountPtr<MeshResource> phx::ImportGltfMesh(const char* path)
{
    // TODO: real import. Construct via RefCountPtr<MeshResource>::Create()
    // (or ::Create(new MeshResource()) if you need to touch the object
    // before handing it back) -- MeshResource starts with ref_counter = 1,
    // and Create()/Attach() take ownership of that existing reference
    // instead of adding a second one on top of it.
    return nullptr;
}
