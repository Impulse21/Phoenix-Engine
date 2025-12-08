#pragma once

#include <stdint.h>
#include <algorithm>
#include <hlsl++.h>

namespace phx::math
{
	constexpr float Saturate(float x) { return std::min(std::max(x, 0.0f), 1.0f); }
	constexpr float cMaxFloat = std::numeric_limits<float>::max();
	constexpr float cMinFloat = std::numeric_limits<float>::lowest();
	template <typename T>
	T LoadInterop(const float* source)
	{
		T result;
		std::memcpy(&result, source, sizeof(T));
		return result;
	}

	inline hlslpp::float4 MatrixToQuaternion(const hlslpp::float3x3& m)
	{
		hlslpp::float4 q;
		float trace = m[0][0] + m[1][1] + m[2][2]; // Calculate the trace of the matrix

		if (trace > 0.0f)
		{
			// This is the most common case, with a positive trace.
			float s = 0.5f / sqrtf(trace + 1.0f);
			q.w = 0.25f / s;
			q.x = (m[2][1] - m[1][2]) * s;
			q.y = (m[0][2] - m[2][0]) * s;
			q.z = (m[1][0] - m[0][1]) * s;
		}
		else
		{
			// Handle cases where the trace is not positive, to maintain numerical stability.
			if (m[0][0] > m[1][1] && m[0][0] > m[2][2])
			{
				// The X component of the quaternion will be largest.
				float s = 2.0f * sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]);
				q.w = (m[2][1] - m[1][2]) / s;
				q.x = 0.25f * s;
				q.y = (m[0][1] + m[1][0]) / s;
				q.z = (m[0][2] + m[2][0]) / s;
			}
			else if (m[1][1] > m[2][2])
			{
				// The Y component of the quaternion will be largest.
				float s = 2.0f * sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]);
				q.w = (m[0][2] - m[2][0]) / s;
				q.x = (m[0][1] + m[1][0]) / s;
				q.y = 0.25f * s;
				q.z = (m[1][2] + m[2][1]) / s;
			}
			else
			{
				// The Z component of the quaternion will be largest.
				float s = 2.0f * sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]);
				q.w = (m[1][0] - m[0][1]) / s;
				q.x = (m[0][2] + m[2][0]) / s;
				q.y = (m[1][2] + m[2][1]) / s;
				q.z = 0.25f * s;
			}
		}
		return q;
	}

	inline bool Decompose(const hlslpp::float4x4& transform, hlslpp::float3& out_scale, hlslpp::float4& out_rotation, hlslpp::float3& out_translation)
	{

		// 1. Extract Translation from the last row
		out_translation = hlslpp::float3(transform[3][0], transform[3][1], transform[3][2]);

		// 2. Extract Scale from the length of the basis vectors (the first three rows)
		out_scale.x = hlslpp::length(hlslpp::float3(transform[0][0], transform[0][1], transform[0][2]));
		out_scale.y = hlslpp::length(hlslpp::float3(transform[1][0], transform[1][1], transform[1][2]));
		out_scale.z = hlslpp::length(hlslpp::float3(transform[2][0], transform[2][1], transform[2][2]));

		// Check for a degenerate matrix
		if (out_scale.x == 0.0f || out_scale.y == 0.0f || out_scale.z == 0.0f)
		{
			return false;
		}

		// 3. Extract Rotation by removing the scale from the basis vectors.
		// The result is the final 3x3 rotation matrix.
		hlslpp::float3x3 rotation_matrix;
		rotation_matrix[0][0] = transform[0][0] / out_scale.x;
		rotation_matrix[0][1] = transform[0][1] / out_scale.x;
		rotation_matrix[0][2] = transform[0][2] / out_scale.x;
		
		rotation_matrix[1][0] = transform[1][0] / out_scale.y;
		rotation_matrix[1][1] = transform[1][1] / out_scale.y;
		rotation_matrix[1][2] = transform[1][2] / out_scale.y;
		
		rotation_matrix[2][0] = transform[2][0] / out_scale.z;
		rotation_matrix[2][1] = transform[2][1] / out_scale.z;
		rotation_matrix[2][2] = transform[2][2] / out_scale.z;

		out_rotation = MatrixToQuaternion(rotation_matrix);

		return true;
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

	constexpr uint64_t GetPreviousPowerOfTwo(uint64_t x)
	{
		if (x == 0) return 0;
		--x;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32u;
		return x - (x >> 1);
	}

	struct BoundingSphere
	{
		hlslpp::float3 centre = hlslpp::float3(0.0f);
		hlslpp::float1 radius = hlslpp::float1(0.0f);

		BoundingSphere Union(const BoundingSphere& rhs) const
		{
			using namespace hlslpp;

			float1 rad_a = radius;
			if (rad_a == 0.0f)
				return rhs;

			float1 rad_b = rhs.radius;
			if (rad_b == 0.0f)
				return *this;

			float3 diff = centre - rhs.centre;
			float1 dist = length(diff);

			// Safe normalize vector between sphere centers
			diff = (float)dist < 1e-6f ? float3(1.0, 0.0, 0.0) : diff * rcp(dist);

			float3 extreme_a = centre + diff * hlslpp::max(rad_a, rad_b - dist);
			float3 extreme_b = rhs.centre - diff * hlslpp::max(rad_b, rad_a - dist);

			return {
				.centre = (extreme_a + extreme_b) * 0.5f,
				.radius = length(extreme_a - extreme_b) * 0.5f
			};
		}
	};

	struct AxisAlignedBox
	{
		hlslpp::float3 min = hlslpp::float3(0.0f);
		hlslpp::float3 max = hlslpp::float3(0.0f);

		void AddPoint(hlslpp::float3 point)
		{
			min = hlslpp::min(point, min);
			max = hlslpp::max(point, max);
		}

		void AddBoundingBox(AxisAlignedBox const& box)
		{
			AddPoint(box.min);
			AddPoint(box.max);
		}
	};

#if false
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
			return AABB(math::Min(a.Min, b.Min), math::Max(a.Max, b.Max));
		}
	};

	struct Frustum
	{
		DirectX::XMFLOAT4 Planes[6];

		Frustum() {}
		Frustum(DirectX::XMMATRIX const& matrix, bool isReverseProjection = false);

		DirectX::XMFLOAT3 GetCorner(int index) const;
		DirectX::XMVECTOR GetCornerV(int index) const;

		DirectX::XMFLOAT4& GetNearPlane() { return this->Planes[0]; }
		DirectX::XMFLOAT4& GetFarPlane() { return this->Planes[1]; }
		DirectX::XMFLOAT4& GetLeftPlane() { return this->Planes[2]; }
		DirectX::XMFLOAT4& GetRightPlane() { return this->Planes[3]; }
		DirectX::XMFLOAT4& GetTopPlane() { return this->Planes[4]; }
		DirectX::XMFLOAT4& GetBottomPlane() { return this->Planes[5]; }

		const DirectX::XMFLOAT4& GetNearPlane() const { return this->Planes[0]; }
		const DirectX::XMFLOAT4& GetFarPlane() const { return this->Planes[1]; }
		const DirectX::XMFLOAT4& GetLeftPlane() const { return this->Planes[2]; }
		const DirectX::XMFLOAT4& GetRightPlane() const { return this->Planes[3]; }
		const DirectX::XMFLOAT4& GetTopPlane() const { return this->Planes[4]; }
		const DirectX::XMFLOAT4& GetBottomPlane() const { return this->Planes[5]; }

		bool CheckBoxFast(AABB const& aabb) const;
	};
#endif
}