#pragma once

#include <memory>

#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::RHI
{
	class IRHIFactory
	{
	public:
		virtual bool IsSupported() = 0;
		virtual bool IsSupported(FeatureLevel requestedFeatureLevel) = 0;

		virtual std::unique_ptr<IPhxRHI> CreateRHI() = 0;

		virtual ~IRHIFactory() {};
	};

	enum class RHIType
	{
		D3D12
	};

	class RHIFactoryProvider
	{
	public:
		std::unique_ptr<IRHIFactory> CreateRHIFactory(RHIType type);
	};
}

