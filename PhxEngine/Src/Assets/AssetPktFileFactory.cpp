#include <PhxEngine/Assets/AssetPktFileFactory.h>

#include <PhxEngine/Assets/PhxArchFile.h>
#include <PhxEngine/RHI/PhxRHI_D3D12.h>
#include <PhxEngine/Assets/PhxArchFile.h>

#include "PhxPktFile_DStorage.h"


using namespace PhxEngine;
using namespace PhxEngine::Assets;


namespace
{
	class DStoragePktFileFactory : public IPktFileFactory
	{
	public:
		std::unique_ptr<PhxPktFile> Create(std::string const& filename) override
		{
			return std::make_unique<PhxPktFile_DStorage>(filename);
		}
	};
}
std::unique_ptr<IPktFileFactory> PhxEngine::Assets::PktFileFactoryProvider::CreateDStorageFactory()
{
	return std::make_unique<DStoragePktFileFactory>();
}
