#pragma once

#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	struct AABB
	{
	};

	struct Frustum
	{
		DirectX::XMFLOAT4 Faces[6] = {};

		Frustum Initialize(DirectX::XMMATRIX const& viewProjection);

		bool CheckPoint(DirectX::XMFLOAT3 const& point) const;
		bool CheckSphere(DirectX::XMFLOAT3 const& point, float raidus) const;

		enum class BoxFrustumIntersect
		{
			Outside,
			Intersects,
			Inside,
		};

		BoxFrustumIntersect CheckBox(const AABB& box) const;
		bool CheckBoxFast(const AABB& box) const;

		const DirectX::XMFLOAT4& GetNearPlane() const;
		const DirectX::XMFLOAT4& GetFarPlane() const;
		const DirectX::XMFLOAT4& GetLeftPlane() const;
		const DirectX::XMFLOAT4& GetRightPlane() const;
		const DirectX::XMFLOAT4& GetTopPlane() const;
		const DirectX::XMFLOAT4& GetBottomPlane() const;
	};
}