#include <PhxEngine/Resource/ResourceManager.h>

#include <vector>
#include <memory>
#include <unordered_map>

#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Core/VirtualFileSystem.h>
#include <PhxEngine/Core/StringHash.h>
#include <PhxEngine/Resource/Mesh.h>

using namespace PhxEngine;

namespace
{
	std::vector<std::string> m_registeredExtension;
	std::unordered_map<StringHash, std::unique_ptr<IResourceFileHanlder>> m_handlerLUT;
	RootFileSystem m_rootFs;
}

void PhxEngine::ResourceManager::Startup()
{
	MeshResourceRegistry::Ptr = new MeshResourceRegistry;
	ResourceManager::RegisterResourceHandler(std::make_unique<MeshResourceFileHandler>());
}

void PhxEngine::ResourceManager::Shutdown()
{
	delete MeshResourceRegistry::Ptr;
}

void PhxEngine::ResourceManager::RegisterResourceHandler(std::unique_ptr<IResourceFileHanlder>&& resourceHandler)
{
	std::string_view fileExten = resourceHandler->GetFileExtension();
	StringHash extensionHash = StringHash(fileExten);
	auto itr = m_handlerLUT.find(extensionHash);
	if (itr == m_handlerLUT.end())
	{
		m_handlerLUT[extensionHash] = std::move(resourceHandler);
		m_registeredExtension.push_back(std::string(fileExten));
	}
}

void PhxEngine::ResourceManager::MountPath(std::string const& path)
{
}

void PhxEngine::ResourceManager::RegisterPath(std::string const& directory)
{
	NativeFileSystem fs;
	fs.EnumerateFiles(directory, m_registeredExtension, [](std::string_view file) {
		
		StringHash extensionHash(FileSystem::GetFileExt(file));
		auto itr = m_handlerLUT.find(extensionHash);
		if (itr != m_handlerLUT.end())
		{
			m_handlerLUT[extensionHash]->RegisterResourceFile(file);
		}
	});
}

void PhxEngine::ResourceManager::RegisterPack(std::string const& filename)
{
	// TODO
}
