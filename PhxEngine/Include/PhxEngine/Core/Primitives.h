#pragma once

#include <DirectXMath.h>

namespace PhxEngine::Core
{
	struct AABB
	{
		DirectX::XMFLOAT3 Min;
		DirectX::XMFLOAT3 Max;

		AABB(
			const DirectX::XMFLOAT3& min = DirectX::XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
			const DirectX::XMFLOAT3& max = DirectX::XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest())
		) : Min(min), Max(max) {}


		DirectX::XMFLOAT3 GetCenter() const;
		constexpr bool IsValid() const
		{
			if (this->Min.x > this->Max.x || this->Min.y > this->Max.y || this->Min.z > this->Max.z)
				return false;
			return true;
		}

		AABB Transform(DirectX::XMMATRIX const& transform) const;
	};
	struct Frustum
	{
		DirectX::XMFLOAT4 Planes[6];

		Frustum() {};
		Frustum(DirectX::XMMATRIX const& matrix, bool isReverseProjection = false);

		DirectX::XMFLOAT3 GetCorner(int index) const;
		DirectX::XMVECTOR GetCornerV(int index) const;

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

		bool CheckBoxFast(AABB const& aabb) const;
	};

}