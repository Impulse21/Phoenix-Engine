#include "phxpch.h"
#include "PhxRHI_Dx12.h"

#include "GraphicsDevice.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;

// DirectX Aligily SDK
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 606; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\"; }

std::unique_ptr<IGraphicsDevice> PhxEngine::RHI::Dx12::Factory::CreateDevice()
{
	return std::make_unique<GraphicsDevice>();
}

IGraphicsDevice* PhxEngine::RHI::DeviceFactory::CreateDx12Device()
{
	return new GraphicsDevice();
}
