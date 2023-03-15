#pragma once

#include <DirectXMath.h>
#include <PhxEngine/Core/Math.h>

namespace PhxEngine::Core
{
	struct Sphere
	{
		DirectX::XMFLOAT3 Centre;
		float Radius;

		Sphere() : Centre(XMFLOAT3(0, 0, 0)), Radius(0) {}
		Sphere(const XMFLOAT3& c, float r) : Centre(c), Radius(r) {}
		Sphere(
			const DirectX::XMFLOAT3& min = DirectX::XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
			const DirectX::XMFLOAT3& max = DirectX::XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()))
		{
			DirectX::XMVECTOR minV = XMLoadFloat3(&min);
			DirectX::XMVECTOR maxV = XMLoadFloat3(&max);

			DirectX::XMVECTOR centre = DirectX::XMVectorAdd(minV, maxV) / 2.0f;
			DirectX::XMVECTOR radius = DirectX::XMVectorMax(DirectX::XMVector3Length(DirectX::XMVectorSubtract(maxV, centre)), DirectX::XMVectorSubtract(centre, minV));
		}

	};

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
		inline DirectX::XMFLOAT3 GetCorner(size_t index) const
		{
			switch (index)
			{
			case 0:
				return { this->Min.x, this->Min.y, this->Max.z };
			case 1:
				return { this->Max.x, this->Min.y, this->Max.z };
			case 2:
				return { this->Min.x, this->Max.y, this->Max.z };
			case 3:
				return this->Max;
			case 4:
				return this->Min;
			case 5:
				return { this->Max.x, this->Min.y, this->Min.z };
			case 6:
				return { this->Min.x, this->Max.y, this->Min.z };
			case 7:
				return { this->Max.x, this->Max.y, this->Min.z };
			default:
				assert(0);
				return { 0.0f, 0.0f, 0.0f };
			}
		}

		static AABB Merge(AABB const& a, AABB const& b)
		{
			return AABB(Core::Math::Min(a.Min, b.Min), Core::Math::Max(a.Max, b.Max));
		}
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