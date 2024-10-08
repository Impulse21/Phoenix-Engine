#pragma once

#include <DirectXMath.h>
#include <algorithm>


namespace phx::Math
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

	struct Sphere
	{
		DirectX::XMFLOAT3 Centre;
		float Radius;

		Sphere(
			const DirectX::XMFLOAT3& min = DirectX::XMFLOAT3(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()),
			const DirectX::XMFLOAT3& max = DirectX::XMFLOAT3(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()))
		{
			using namespace DirectX;
			DirectX::XMVECTOR minV = XMLoadFloat3(&min);
			DirectX::XMVECTOR maxV = XMLoadFloat3(&max);

			DirectX::XMVECTOR centre = DirectX::XMVectorAdd(minV, maxV) / 2.0f;
			DirectX::XMVECTOR radius = DirectX::XMVectorMax(DirectX::XMVector3Length(DirectX::XMVectorSubtract(maxV, centre)), DirectX::XMVector3Length(DirectX::XMVectorSubtract(centre, minV)));

			DirectX::XMStoreFloat3(&this->Centre, centre);
			DirectX::XMStoreFloat(&this->Radius, radius);
		}

		Sphere(DirectX::XMFLOAT3 const& c, float r)
			: Centre(c)
			, Radius(r)
		{}

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
			return AABB(Math::Min(a.Min, b.Min), Math::Max(a.Max, b.Max));
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