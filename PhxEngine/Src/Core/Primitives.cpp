#include "C:/Users/dipao/source/repos/Impulse21/Phoenix-Engine/Build/PhxEngine/CMakeFiles/PhxEngine.dir/Debug/cmake_pch.hxx"
#include "PhxEngine/Core/Primitives.h"
#include <algorithm>

using namespace PhxEngine::Core;
using namespace DirectX;

PhxEngine::Core::Frustum::Frustum(DirectX::XMMATRIX const& viewProjection, bool isReverseProjection)
{
	// We are interested in columns of the matrix, so transpose because we can access only rows:
	const XMMATRIX mat = XMMatrixTranspose(viewProjection);

	// Near plane:
	XMStoreFloat4(&this->Planes[0], XMPlaneNormalize(mat.r[2]));

	// Far plane:
	XMStoreFloat4(&this->Planes[1], XMPlaneNormalize(mat.r[3] - mat.r[2]));

	if (isReverseProjection)
	{
		std::swap(this->Planes[0], this->Planes[1]);
	}

	// Left plane:
	XMStoreFloat4(&this->Planes[2], XMPlaneNormalize(mat.r[3] + mat.r[0]));

	// Right plane:
	XMStoreFloat4(&this->Planes[3], XMPlaneNormalize(mat.r[3] - mat.r[0]));

	// Top plane:
	XMStoreFloat4(&this->Planes[4], XMPlaneNormalize(mat.r[3] - mat.r[1]));

	// Bottom plane:
	XMStoreFloat4(&this->Planes[5], XMPlaneNormalize(mat.r[3] + mat.r[1]));
}
