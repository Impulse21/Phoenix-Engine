#include "PhxResource/PhxResource_pch.h"

#include "PhxCore/Base.h"
#include "ResourceManger.h"
#include "PakManager.h"

using namespace phx;

static std::unique_ptr<PakFileFormat::Header> g_testHeader = {};

void ResourceManger::Initialize(std::filesystem::path const& resourcePath)
{
	ms_fileSytem = phx::FileSystemFactory::CreateResourceFileSystem();

	ms_fileSytem->Mount("res:/", resourcePath);
}

void ResourceManger::RegisterPakFiles(Span<std::filesystem::path> pakFiles)
{
	for (auto& pakFile : pakFiles)
	{
		PHX_CORE_INFO("Registering PakFile {0}.", pakFile.generic_string().c_str());
		RegisterPakFile(pakFile);
	}

	ms_fileSytem->SubmitRequests();
}

void phx::ResourceManger::RegisterPakFile(std::filesystem::path const& pakFile)
{
	// Get filename of pak file
	StringHash pakFileId(pakFile.stem().generic_string());

	FileHandle fileHandle = ms_fileSytem->Open(pakFile);
	
	g_testHeader = std::make_unique<PakFileFormat::Header>();

#if false
	ms_fileSytem->EnqueueRead(fileHandle, 0, g_testHeader.get(), [&] {
		PHX_CORE_INFO(
			"Header Loaded info: \n\tVersion:{0}\n\tBuild:{1}\n\tNumEntries:{2}",
			g_testHeader->Version,
			g_testHeader->BuildNumber,
			g_testHeader->NumEntries);
	});
#endif 
	ms_fileSytem->Close(fileHandle);
}

RefCountPtr<IResource> ResourceManger::Get(const char* name, const char* ext)
{
	StringHash filenameHash(std::format("{}.{}", name, ext));

	std::scoped_lock _(ms_mutex);
	auto itr = ms_cache.find(filenameHash);
	if (itr != ms_cache.end())
		return itr->second;

	auto factoryItr = ms_resourceFactories.find(StringHash(ext));
	if (factoryItr == ms_resourceFactories.end())
	{
		PHX_CORE_ERROR("Unknown File extension encounters: '{0}'", ext);
		return nullptr;
	}

	ms_cache[filenameHash] = factoryItr->second->Create(filenameHash, name);
	ms_cache[filenameHash]->StartMetadataLoad();
	return ms_cache[filenameHash];
}