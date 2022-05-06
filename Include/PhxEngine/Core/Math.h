#pragma once

#include <DirectXMath.h>
#include <algorithm>
#include <PhxEngine/RHI/PhxRHI.h>

namespace PhxEngine::Core::Math
{
	constexpr float Saturate(float x) { return std::min(std::max(x, 0.0f), 1.0f); }

	inline uint32_t PackColour(DirectX::XMFLOAT4 const& colour)
	{
		uint32_t retVal = 0;
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.x) * 255.0f) << 0);
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.y) * 255.0f) << 8);
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.z) * 255.0f) << 16);
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.y) * 255.0f) << 24);

		return retVal;
	}

	inline uint32_t PackColour(PhxEngine::RHI::Color const& colour)
	{
		uint32_t retVal = 0;
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.R) * 255.0f) << 0);
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.G) * 255.0f) << 8);
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.B) * 255.0f) << 16);
		retVal |= (uint32_t)((uint8_t)(Saturate(colour.A) * 255.0f) << 24);

		return retVal;
	}
}