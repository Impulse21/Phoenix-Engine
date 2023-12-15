#include <PhxEngine/Engine/World.h>

#include <filesystem>

using namespace PhxEngine;

namespace
{
	class GltfWorldLoader : public World::IWorldLoader
	{
	public:
		bool LoadWorld(std::string const& filename, Core::IFileSystem* fileSystem, World::World& outWorld) override
		{
			return true;
		}
	};
}
std::unique_ptr<World::IWorldLoader> PhxEngine::World::WorldLoaderFactory::Create()
{
	return std::make_unique<GltfWorldLoader>();
}
