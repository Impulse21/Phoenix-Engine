#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "GfxDeviceFactory.h"

#define ENABLE_D3D12 1

#if ENABLE_D3D12
#include "RHID3D12/D3D12DeviceFactory.h"
#endif

using namespace PhxEngine::RHI;

// TODO: Only expose what is supported via defines or build.
std::unique_ptr<IGraphicsDeviceFactory> PhxEngine::RHI::GfxDeviceFactoryProvider::CreatGfxDeviceFactory(RHIType type)
{
	switch (type)
	{
#if ENABLE_D3D12
	case RHIType::D3D12:
		return std::make_unique<D3D12::D3D12GraphicsDeviceFactory>();
#endif
	default:
		return nullptr;
	}
}
