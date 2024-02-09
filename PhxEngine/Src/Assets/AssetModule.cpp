#include <PhxEngine/Assets/AssetModule.h>

#include <PhxEngine/Assets/AssetRegistry.h>
#include <PhxEngine/Assets/AssetPktFileFactory.h>

#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Assets/PhxPktFileManager.h>

using Json = nlohmann::json;

void PhxEngine::Assets::AssetModule::InitializeWindows(PhxEngine::RHI::GfxDevice* gfxDevice, nlohmann::json const& configuration)
{
	assert(gfxDevice->GetApi() == RHI::GraphicsAPI::DX12);

	// Create Registries;
	MeshRegistry::Ptr = phx_new(Assets::MeshRegistry);

	// Create PktFile Manager.
	std::unique_ptr<IPktFileFactory> factory = PktFileFactoryProvider::CreateDStorageFactory();
	Assets::PhxPktFileManager::Initialize(std::move(factory));

}

void PhxEngine::Assets::AssetModule::Finalize()
{
	Assets::PhxPktFileManager::Finalize();
	phx_delete(MeshRegistry::Ptr);

}
