#include "PhxWorld/PhxWorld_pch.h"

#include <PhxCore/VFS.h>

#include "WorldSerializer.h"
#include "World.h"
#include "WorldComponents.h"

#include <fstream>

#include <PhxWorld/Entity.h>

#include <entt/entt.hpp>
#include <yaml-cpp/yaml.h>

using namespace phx;


bool WorldSerializer::Save(IFileSystem* /*fs*/, const char* /*filename*/, World& /*world*/)
{
	return true;
}

void phx::WorldSerializer::Load(IFileSystem* /*fs*/, const char* /*filename*/, World& /*world*/)
{
}