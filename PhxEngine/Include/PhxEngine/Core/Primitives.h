#pragma once

#include <DirectXMath.h>

namespace PhxEngine::Core
{
	struct Frustum
	{
		DirectX::XMFLOAT4 Planes[6];

		Frustum() {};
		Frustum(DirectX::XMMATRIX const& viewProjection, bool isReverseProjection = false);

		DirectX::XMFLOAT4& GetNearPlane() { return this->Planes[0]; };
		DirectX::XMFLOAT4& GetFarPlane() { return this->Planes[1]; };
		DirectX::XMFLOAT4& GetLeftPlane() { return this->Planes[2]; };
		DirectX::XMFLOAT4& GetRightPlane() { return this->Planes[3]; };
		DirectX::XMFLOAT4& GetTopPlane() { return this->Planes[4]; };
		DirectX::XMFLOAT4& GetBottomPlane() { return this->Planes[5]; };

		const DirectX::XMFLOAT4& GetNearPlane() const { return this->Planes[0]; };
		const DirectX::XMFLOAT4& GetFarPlane() const { return this->Planes[1]; };
		const DirectX::XMFLOAT4& GetLeftPlane() const { return this->Planes[2]; };
		const DirectX::XMFLOAT4& GetRightPlane() const { return this->Planes[3]; };
		const DirectX::XMFLOAT4& GetTopPlane() const { return this->Planes[4]; };
		const DirectX::XMFLOAT4& GetBottomPlane() const { return this->Planes[5]; };

	};
}