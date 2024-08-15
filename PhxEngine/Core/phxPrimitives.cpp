#include "pch.h"
#include "phxPrimitives.h"
#include "phxMath.h"
#include <algorithm>

using namespace phx;
using namespace DirectX;

Frustum::Frustum(DirectX::XMMATRIX const& m, bool isReverseProjection)
{
	// We are interested in columns of the m, so transpose because we can access only rows:
	const XMMATRIX mat = XMMatrixTranspose(m);
	// https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrix.pdf
#if true
	// Near plane:
	XMStoreFloat4(&this->GetNearPlane(), XMPlaneNormalize(mat.r[2]));

	// Far plane:
	XMStoreFloat4(&this->GetFarPlane(), XMPlaneNormalize(mat.r[3] - mat.r[2]));

	if (isReverseProjection)
	{
		std::swap(this->GetNearPlane(), this->GetFarPlane());
	}

	// Left plane:
	XMStoreFloat4(&this->GetLeftPlane(), XMPlaneNormalize(mat.r[3] + mat.r[0]));

	// Right plane:
	XMStoreFloat4(&this->GetRightPlane(), XMPlaneNormalize(mat.r[3] - mat.r[0]));

	// Top plane:
	XMStoreFloat4(&this->GetTopPlane(), XMPlaneNormalize(mat.r[3] - mat.r[1]));

	// Bottom plane:
	XMStoreFloat4(&this->GetBottomPlane(), XMPlaneNormalize(mat.r[3] + mat.r[1]));

#else
	DirectX::XMVECTOR negRow2 = DirectX::XMVectorNegate(mat.r[2]);
	DirectX::XMVECTOR negRow3 = DirectX::XMVectorNegate(mat.r[3]);
	// Near plane:
	XMStoreFloat4(&this->GetNearPlane(), XMPlaneNormalize(XMVectorSelect(m.r[2], negRow2, g_XMSelect1110)));

	// Far plane:
	XMStoreFloat4(&this->GetFarPlane(), XMPlaneNormalize(negRow3 + mat.r[2]));

	if (isReverseProjection)
	{
		std::swap(this->GetNearPlane(), this->GetFarPlane());
	}

	// Left plane:
	XMStoreFloat4(&this->GetLeftPlane(), XMPlaneNormalize(negRow3 - mat.r[0]));

	// Right plane:
	XMStoreFloat4(&this->GetRightPlane(), XMPlaneNormalize(mat.r[3] + mat.r[0]));

	// Top plane:
	XMStoreFloat4(&this->GetTopPlane(), XMPlaneNormalize(negRow3 + mat.r[1]));

	// Bottom plane:
	XMStoreFloat4(&this->GetBottomPlane(), XMPlaneNormalize(negRow3 - mat.r[1]));

	*this = normalize();
#endif
}

// Construct plane points using Planer intersection.
// https://donw.io/post/frustum-point-extraction/
DirectX::XMVECTOR Frustum::GetCornerV(int index) const
{
	const DirectX::XMFLOAT4& a = (index & 1) ? this->GetRightPlane() : this->GetLeftPlane();
	const DirectX::XMFLOAT4& b = (index & 2) ? this->GetTopPlane() : this->GetBottomPlane();
	const DirectX::XMFLOAT4& c = (index & 4) ? this->GetFarPlane() : this->GetNearPlane();

	return math::PlaneIntersects(a, b, c);
}

// From Wicked Engine 
bool Frustum::CheckBoxFast(AABB const& aabb) const
{
	if (!aabb.IsValid())
	{
		return false;
	}

	DirectX::XMVECTOR max = DirectX::XMLoadFloat3(&aabb.Max);
	DirectX::XMVECTOR min = DirectX::XMLoadFloat3(&aabb.Min);
	DirectX::XMVECTOR zero = DirectX::XMVectorZero();

	for (auto& p : this->Planes)
	{
		DirectX::XMVECTOR planeV = DirectX::XMLoadFloat4(&p);
		DirectX::XMVECTOR lt = DirectX::XMVectorLess(planeV, zero);
		DirectX::XMVECTOR furthestFromPlane = DirectX::XMVectorSelect(max, min, lt); 

		// Need to understand Plane Dot Coord.
		if (DirectX::XMVectorGetX(DirectX::XMPlaneDotCoord(planeV, furthestFromPlane)) < 0.0f)
		{
			return false;
		}
	}

	return true;
}


DirectX::XMFLOAT3 Frustum::GetCorner(int index) const
{
	DirectX::XMFLOAT3 retVal = {};
	DirectX::XMStoreFloat3(&retVal, this->GetCornerV(index));

	return retVal;
}

DirectX::XMFLOAT3 AABB::GetCenter() const
{
	return DirectX::XMFLOAT3((this->Min.x + this->Max.x) * 0.5f, (this->Min.y + this->Max.y) * 0.5f, (this->Min.z + this->Max.z) * 0.5f);
}

AABB AABB::Transform(DirectX::XMMATRIX const& transform) const
{
	// Transform the AABB's 8 corners into transform space
	std::array<DirectX::XMVECTOR, 8> corners =
	{
		DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&this->Min), transform),
		DirectX::XMVector3Transform(DirectX::XMVectorSet(this->Min.x, this->Max.y, this->Min.z, 1), transform),
		DirectX::XMVector3Transform(DirectX::XMVectorSet(this->Min.x, this->Max.y, this->Max.z, 1), transform),
		DirectX::XMVector3Transform(DirectX::XMVectorSet(this->Min.x, this->Min.y, this->Max.z, 1), transform),
		DirectX::XMVector3Transform(DirectX::XMVectorSet(this->Max.x, this->Min.y, this->Max.z, 1), transform),
		DirectX::XMVector3Transform(DirectX::XMVectorSet(this->Max.x, this->Min.y, this->Min.z, 1), transform),
		DirectX::XMVector3Transform(DirectX::XMVectorSet(this->Max.x, this->Max.y, this->Min.z, 1), transform),
		DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&this->Max), transform),
	};
	
	DirectX::XMVECTOR transformedMinV = corners[0];
	DirectX::XMVECTOR transformedMaxV = corners[0];
	transformedMinV = DirectX::XMVectorMin(transformedMinV, corners[1]);
	transformedMaxV = DirectX::XMVectorMax(transformedMaxV, corners[1]);
	transformedMinV = DirectX::XMVectorMin(transformedMinV, corners[2]);
	transformedMaxV = DirectX::XMVectorMax(transformedMaxV, corners[2]);
	transformedMinV = DirectX::XMVectorMin(transformedMinV, corners[3]);
	transformedMaxV = DirectX::XMVectorMax(transformedMaxV, corners[3]);
	transformedMinV = DirectX::XMVectorMin(transformedMinV, corners[4]);
	transformedMaxV = DirectX::XMVectorMax(transformedMaxV, corners[4]);
	transformedMinV = DirectX::XMVectorMin(transformedMinV, corners[5]);
	transformedMaxV = DirectX::XMVectorMax(transformedMaxV, corners[5]);
	transformedMinV = DirectX::XMVectorMin(transformedMinV, corners[6]);
	transformedMaxV = DirectX::XMVectorMax(transformedMaxV, corners[6]);
	transformedMinV = DirectX::XMVectorMin(transformedMinV, corners[7]);
	transformedMaxV = DirectX::XMVectorMax(transformedMaxV, corners[7]);

	DirectX::XMFLOAT3 transformedMin;
	DirectX::XMStoreFloat3(&transformedMin, transformedMinV);

	DirectX::XMFLOAT3 transformedMax;
	DirectX::XMStoreFloat3(&transformedMax, transformedMaxV);

	return AABB(transformedMin, transformedMax);
}
