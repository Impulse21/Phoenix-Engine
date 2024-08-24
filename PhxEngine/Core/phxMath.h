#pragma once

#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <algorithm>

namespace phx::math
{

	static constexpr DirectX::XMFLOAT4X4 cIdentityMatrix = DirectX::XMFLOAT4X4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	constexpr float Saturate(float x) { return std::min(std::max(x, 0.0f), 1.0f); }
	constexpr float cMaxFloat = std::numeric_limits<float>::max();
	constexpr float cMinFloat = std::numeric_limits<float>::lowest();

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

	inline DirectX::XMFLOAT3 Min(DirectX::XMFLOAT3 const& a, DirectX::XMFLOAT3 const& b)
	{
		return DirectX::XMFLOAT3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
	}

	inline DirectX::XMFLOAT3 Max(DirectX::XMFLOAT3 const& a, DirectX::XMFLOAT3 const& b)
	{
		return DirectX::XMFLOAT3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
	}

	// https://donw.io/post/frustum-point-extraction/
	inline DirectX::XMVECTOR PlaneIntersects(
		DirectX::XMFLOAT4 const& plane1,
		DirectX::XMFLOAT4 const& plane2,
		DirectX::XMFLOAT4 const& plane3)
	{
		auto m = DirectX::XMMatrixSet(
			plane1.x, plane1.y, plane1.z, 0.0f,
			plane2.x, plane2.y, plane2.z, 0.0f,
			plane3.x, plane3.y, plane3.z, 0.0f,
			0.f, 0.f, 0.f, 1.0f);

		DirectX::XMVECTOR d = DirectX::XMVectorSet(plane1.w, plane2.w, plane3.w, 1.0f);
		return DirectX::XMVector3Transform(d, DirectX::XMMatrixInverse(nullptr, m));
	}

	constexpr uint64_t GetNextPowerOfTwo(uint64_t x)
	{
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32u;
		return ++x;
	}
}