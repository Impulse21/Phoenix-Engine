#pragma once


#include <PhxCore/UUID.h>
#include <PhxData/DataPtr.h>

#include <sstream>
#include <entt/entt.hpp>

namespace phx
{
	class IFileSystem;
	class World;

	namespace WorldSerializer
	{
		bool Save(phx::IFileSystem* fs, const char* filename, phx::World& world);
		data::RefPtr<phx::World> Load(phx::IFileSystem* fs, const char* filename);
	}
}