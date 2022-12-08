#pragma once

#include <DirectXMath.h>
#include <algorithm>

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

	inline float Distance(DirectX::XMVECTOR const& v1, DirectX::XMVECTOR const& v2)
	{
		auto subVector = DirectX::XMVectorSubtract(v1, v2);
		auto length = DirectX::XMVector3Length(subVector);

		float distance = 0.0f;
		DirectX::XMStoreFloat(&distance, length);
		return distance;
	}

	inline float Distance(DirectX::XMFLOAT3 const& f1, DirectX::XMFLOAT3 const& f2)
	{
		auto v1 = DirectX::XMLoadFloat3(&f1);
		auto v2 = DirectX::XMLoadFloat3(&f2);
		return Distance(v1, v2);
	}
}