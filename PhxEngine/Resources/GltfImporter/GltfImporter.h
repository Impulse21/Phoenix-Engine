#pragma once

#include <PhxEngine/Core/RefCountPtr.h>
#include <PhxEngine/Resources/MeshResource.h>

namespace phx
{
    // TODO: Need to consider how to handle textures
    // and scene data. Figured I would for now apply the sceen info transforms right
    // into the vertex data so it because Identity for the mesh.
    // The textures still need to be handled in some way.
    RefCountPtr<MeshResource> ImportGltfMesh(const char* path);
}