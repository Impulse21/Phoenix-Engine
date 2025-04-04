#pragma once

namespace phx
{
	class World;
	class IFileSystem;

	namespace WorldSerializer
	{
		bool Save(IFileSystem* fs, const char* filename, World& world);
		void Load(IFileSystem* fs, const char* filename, World& world);
	}
}