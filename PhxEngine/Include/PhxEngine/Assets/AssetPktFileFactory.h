#pragma once

#include <memory>
#include <string>

namespace PhxEngine::RHI
{
	namespace D3D12
	{
		class ID3D12GfxDevice;
	}
}
namespace PhxEngine::Assets
{
	class PhxPktFile;

	class IPktFileFactory
	{
	public:
		virtual std::unique_ptr<PhxPktFile> Create(std::string const& filename) = 0;
	};

	class PktFileFactoryProvider
	{
	public:
		static std::unique_ptr<IPktFileFactory> CreateDStorageFactory();
	};

}

