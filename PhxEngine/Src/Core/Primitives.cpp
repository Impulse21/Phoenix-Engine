#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "PhxEngine/Core/Primitives.h"
#include <algorithm>

using namespace PhxEngine::Core;
using namespace DirectX;

PhxEngine::Core::Frustum::Frustum(DirectX::XMMATRIX const& m, bool isReverseProjection)
{
	// We are interested in columns of the matrix, so transpose because we can access only rows:
	const XMMATRIX mat = XMMatrixTranspose(m);

#if false
	// near plane
	DirectX::XMStoreFloat4(&this->GetNearPlane(), XMPlaneNormalize(-mat.r[2]));

	DirectX::XMStoreFloat4(&this->GetFarPlane(), XMPlaneNormalize(-mat.r[3] + mat.r[2]));

	if (isReverseProjection)
	{
		std::swap(this->GetNearPlane(), this->GetFarPlane());
	}

	DirectX::XMStoreFloat4(&this->GetLeftPlane(), XMPlaneNormalize(-mat.r[3] - mat.r[0]));
	DirectX::XMStoreFloat4(&this->GetRightPlane(), XMPlaneNormalize(-mat.r[3] + mat.r[0]));

	DirectX::XMStoreFloat4(&this->GetTopPlane(), XMPlaneNormalize(-mat.r[3] + mat.r[1]));
	DirectX::XMStoreFloat4(&this->GetBottomPlane(), XMPlaneNormalize(-mat.r[3] - mat.r[2]));
#else
	// Near plane:
	XMStoreFloat4(&this->GetNearPlane(), XMPlaneNormalize(mat.r[2]));

	// Far plane:
	XMStoreFloat4(&this->GetFarPlane(), XMPlaneNormalize(mat.r[3] - mat.r[2]));

	// Left plane:
	XMStoreFloat4(&this->GetLeftPlane(), XMPlaneNormalize(mat.r[3] + mat.r[0]));

	// Right plane:
	XMStoreFloat4(&this->GetRightPlane(), XMPlaneNormalize(mat.r[3] - mat.r[0]));

	// Top plane:
	XMStoreFloat4(&this->GetTopPlane(), XMPlaneNormalize(mat.r[3] - mat.r[1]));

	// Bottom plane:
	XMStoreFloat4(&this->GetBottomPlane(), XMPlaneNormalize(mat.r[3] + mat.r[1]));
#endif
}


DirectX::XMVECTOR Frustum::GetCornerV(int index) const
{

	const DirectX::XMFLOAT4& a = (index & 1) ? this->GetRightPlane() : this->GetLeftPlane();
	const DirectX::XMFLOAT4& b = (index & 2) ? this->GetTopPlane() : this->GetBottomPlane();
	const DirectX::XMFLOAT4& c = (index & 4) ? this->GetFarPlane() : this->GetNearPlane();

	auto m = DirectX::XMMatrixSet(
		a.x, a.y, a.z, 0.0f,
		b.x, b.y, a.z, 0.0f,
		c.x, c.y, c.z, 0.0f,
		0.f, 0.f, 0.f, 1.0f);

	DirectX::XMVECTOR d = DirectX::XMVectorSet(a.w, b.w, c.w, 1.0f);
	return DirectX::XMVector3Transform(d, DirectX::XMMatrixInverse(nullptr, m));
}


DirectX::XMFLOAT3 Frustum::GetCorner(int index) const
{
	DirectX::XMFLOAT3 retVal = {};
	DirectX::XMStoreFloat3(&retVal, this->GetCornerV(index));

	return retVal;
}