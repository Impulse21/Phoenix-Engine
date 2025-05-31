#include "GltfFileHandler.h"

#include <PhxWorld/SceneBlueprint.h>

using namespace phx;
using namespace phxed;

phx::RefCountPtr<phx::Resource> phxed::GltfFileHandler::LoadFromPak() const
{
    return phx::RefCountPtr<SceneBlueprint>();
}

phx::RefCountPtr<phx::Resource> phxed::GltfFileHandler::LoadLoose(const char* /*filename*/) const
{
    return phx::RefCountPtr<SceneBlueprint>();
}
