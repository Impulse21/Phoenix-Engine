#pragma once

#include <memory>

#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::RHI
{
	class IGraphicsDeviceFactory
	{
	public:
		virtual bool IsSupported() = 0;
		virtual bool IsSupported(FeatureLevel requestedFeatureLevel) = 0;
		virtual std::unique_ptr<IGraphicsDevice> CreateDevice() = 0;
	};

	enum class RHIType
	{
		D3D12
	};

	class GfxDeviceFactoryProvider
	{
	public:
		std::unique_ptr<IGraphicsDeviceFactory> CreatGfxDeviceFactory(RHIType type);
	};
}

