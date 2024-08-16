#include "pch.h"
#include "phxPrefabFactory.h"

#include "Core/phxMath.h"
#include "World/phxComponents.h"
#include "Renderer/phxRenderComponents.h"


using namespace phx;
using namespace phx::rhi;
using namespace DirectX;

#if false
Entity PrefabFactory::CreateCube(
	rhi::GfxDevice* gfxDevice,
	entt::entity matId,
	float size,
	bool rhsCoord)
{

	// A cube has six faces, each one pointing in a different direction.
	constexpr int FaceCount = 6;

	constexpr XMVECTORF32 faceNormals[FaceCount] =
	{
		{ 0,  0,  1 },
		{ 0,  0, -1 },
		{ 1,  0,  0 },
		{ -1,  0,  0 },
		{ 0,  1,  0 },
		{ 0, -1,  0 },
	};

	constexpr XMFLOAT3 faceColour[] =
	{
		{ 1.0f,  0.0f,  0.0f },
		{ 0.0f,  1.0f,  0.0f },
		{ 0.0f,  0.0f,  1.0f },
	};

	constexpr XMFLOAT2 textureCoordinates[4] =
	{
		{ 1, 0 },
		{ 1, 1 },
		{ 0, 1 },
		{ 0, 0 },
	};

	Entity retVal = this->CreateEntity("Cube Mesh");
	MeshComponent& mesh = retVal.AddComponent<MeshComponent>();
	mesh.Material = matId;

	size /= 2;
	mesh.TotalVertices = FaceCount * 4;
	mesh.TotalIndices = FaceCount * 6;

	mesh.InitializeCpuBuffers(this->GetAllocator());

	size_t vbase = 0;
	size_t ibase = 0;
	for (int i = 0; i < FaceCount; i++)
	{
		XMVECTOR normal = faceNormals[i];

		// Get two vectors perpendicular both to the face normal and to each other.
		XMVECTOR basis = (i >= 4) ? g_XMIdentityR2 : g_XMIdentityR1;

		XMVECTOR side1 = XMVector3Cross(normal, basis);
		XMVECTOR side2 = XMVector3Cross(normal, side1);

		// Six indices (two triangles) per face.
		mesh.Indices[ibase + 0] = static_cast<uint16_t>(vbase + 0);
		mesh.Indices[ibase + 1] = static_cast<uint16_t>(vbase + 1);
		mesh.Indices[ibase + 2] = static_cast<uint16_t>(vbase + 2);

		mesh.Indices[ibase + 3] = static_cast<uint16_t>(vbase + 0);
		mesh.Indices[ibase + 4] = static_cast<uint16_t>(vbase + 2);
		mesh.Indices[ibase + 5] = static_cast<uint16_t>(vbase + 3);

		XMFLOAT3 positon;
		XMStoreFloat3(&positon, (normal - side1 - side2) * size);
		mesh.Positions[vbase + 0] = positon;

		XMStoreFloat3(&positon, (normal - side1 + side2) * size);
		mesh.Positions[vbase + 1] = positon;

		XMStoreFloat3(&positon, (normal + side1 + side2) * size);
		mesh.Positions[vbase + 2] = positon;

		XMStoreFloat3(&positon, (normal + side1 - side2) * size);
		mesh.Positions[vbase + 3] = positon;

		mesh.TexCoords[vbase + 0] = textureCoordinates[0];
		mesh.TexCoords[vbase + 1] = textureCoordinates[1];
		mesh.TexCoords[vbase + 2] = textureCoordinates[2];
		mesh.TexCoords[vbase + 3] = textureCoordinates[3];

		mesh.Colour[vbase + 0] = faceColour[0];
		mesh.Colour[vbase + 1] = faceColour[1];
		mesh.Colour[vbase + 2] = faceColour[2];
		mesh.Colour[vbase + 3] = faceColour[3];

		DirectX::XMStoreFloat3((mesh.Normals + vbase + 0), normal);
		DirectX::XMStoreFloat3((mesh.Normals + vbase + 1), normal);
		DirectX::XMStoreFloat3((mesh.Normals + vbase + 2), normal);
		DirectX::XMStoreFloat3((mesh.Normals + vbase + 3), normal);

		vbase += 4;
		ibase += 6;
	}

	mesh.Flags =
		MeshComponent::Flags::kContainsNormals |
		MeshComponent::Flags::kContainsTexCoords |
		MeshComponent::Flags::kContainsTangents |
		MeshComponent::Flags::kContainsColour;

	mesh.ComputeTangentSpace();

	if (rhsCoord)
	{
		mesh.ReverseWinding();
		mesh.FlipZ();
	}

	// Calculate AABB
	mesh.ComputeBounds();
	mesh.ComputeMeshletData(this->GetAllocator());

	mesh.BuildRenderData(this->GetAllocator(), gfxDevice);
	return retVal;
}

Entity PrefabFactory::CreateSphere(
	rhi::GfxDevice* gfxDevice,
	entt::entity matId,
	float diameter,
	size_t tessellation,
	bool rhcoords)
{

	if (tessellation < 3)
	{
		LOG_CORE_ERROR("tessellation parameter out of range");
		throw std::out_of_range("tessellation parameter out of range");
	}

	Entity retVal = this->CreateEntity("Cube Mesh");
	MeshComponent& mesh = retVal.AddComponent<MeshComponent>();
	mesh.Material = matId;

	float radius = diameter / 2.0f;
	size_t verticalSegments = tessellation;
	size_t horizontalSegments = tessellation * 2;

	mesh.TotalVertices = (verticalSegments + 1) * (horizontalSegments + 1);
	mesh.TotalIndices += verticalSegments * (horizontalSegments + 1) * 6;
	mesh.InitializeCpuBuffers(this->GetAllocator());

	// Create rings of vertices at progressively higher latitudes.
	size_t vbase = 0;
	for (size_t i = 0; i <= verticalSegments; i++)
	{
		float v = 1 - (float)i / verticalSegments;

		float latitude = (i * XM_PI / verticalSegments) - XM_PIDIV2;
		float dy, dxz;

		XMScalarSinCos(&dy, &dxz, latitude);

		// Create a single ring of vertices at this latitude.
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			float u = (float)j / horizontalSegments;

			float longitude = j * XM_2PI / horizontalSegments;
			float dx, dz;

			XMScalarSinCos(&dx, &dz, longitude);

			dx *= dxz;
			dz *= dxz;

			XMVECTOR normal = XMVectorSet(dx, dy, dz, 0);

			XMFLOAT3 positon;
			XMStoreFloat3(&positon, normal * radius);
			mesh.Positions[vbase] = positon;
			mesh.TexCoords[vbase] = { u, v };
			mesh.Normals[vbase] = { dx, dy, dz };
			mesh.Colour[vbase] = { 0.1f, 0.1f, 0.1f };
			vbase++;
		}
	}

	// Fill the index buffer with triangles joining each pair of latitude rings.
	size_t stride = horizontalSegments + 1;
	size_t iBase = 0;
	for (size_t i = 0; i < verticalSegments; i++)
	{
		for (size_t j = 0; j <= horizontalSegments; j++)
		{
			size_t nextI = i + 1;
			size_t nextJ = (j + 1) % stride;

			mesh.Indices[iBase + 0] = static_cast<uint16_t>(i * stride + j);
			mesh.Indices[iBase + 1] = static_cast<uint16_t>(nextI * stride + j);
			mesh.Indices[iBase + 2] = static_cast<uint16_t>(i * stride + nextJ);

			mesh.Indices[iBase + 3] = static_cast<uint16_t>(i * stride + nextJ);
			mesh.Indices[iBase + 4] = static_cast<uint16_t>(nextI * stride + j);
			mesh.Indices[iBase + 5] = static_cast<uint16_t>(nextI * stride + nextJ);
			iBase += 6;
		}
	}

	mesh.Flags =
		MeshComponent::Flags::kContainsNormals |
		MeshComponent::Flags::kContainsTexCoords |
		MeshComponent::Flags::kContainsTangents |
		MeshComponent::Flags::kContainsColour;

	mesh.ComputeTangentSpace();

	if (rhcoords)
	{
		mesh.ReverseWinding();
		mesh.FlipZ();
	}

	mesh.ComputeBounds();
	mesh.ComputeMeshletData(this->GetAllocator());

	mesh.BuildRenderData(this->GetAllocator(), gfxDevice);
	return retVal;
}

Entity PrefabFactory::CreatePlane(
	rhi::GfxDevice* gfxDevice,
	entt::entity matId,
	float width,
	float height,
	bool rhCoords)
{
	constexpr uint32_t PlaneIndices[] =
	{
		0, 3, 1, 1, 3, 2
	};

	Entity retVal = this->CreateEntity("Plane Mesh");
	MeshComponent& mesh = retVal.AddComponent<MeshComponent>();
	mesh.Material = matId;

	mesh.TotalVertices = 4;
	mesh.TotalIndices = _ARRAYSIZE(PlaneIndices);
	mesh.InitializeCpuBuffers(this->GetAllocator());

	mesh.Positions[0] = { -0.5f * width, 0.0f, 0.5f * height };
	mesh.Positions[1] = { 0.5f * width, 0.0f,  0.5f * height };
	mesh.Positions[2] = { 0.5f * width, 0.0f, -0.5f * height };
	mesh.Positions[3] = { -0.5f * width, 0.0f, -0.5f * height };

	mesh.Normals[0] = { 0.0f, 1.0f, 0.0f };
	mesh.Normals[1] = { 0.0f, 1.0f, 0.0f };
	mesh.Normals[2] = { 0.0f, 1.0f, 0.0f };
	mesh.Normals[3] = { 0.0f, 1.0f, 0.0f };

	mesh.TexCoords[0] = { 0, 0 };
	mesh.TexCoords[1] = { 1, 0 };
	mesh.TexCoords[2] = { 1, 1 };
	mesh.TexCoords[3] = { 0, 1 };

	std::memcpy(mesh.Indices, &PlaneIndices, _ARRAYSIZE(PlaneIndices) * sizeof(uint32_t));


	mesh.ComputeTangentSpace();

	mesh.Flags =
		MeshComponent::Flags::kContainsNormals |
		MeshComponent::Flags::kContainsTexCoords |
		MeshComponent::Flags::kContainsTangents |
		MeshComponent::Flags::kContainsColour;

	if (rhcoords)
	{
		mesh.ReverseWinding();
		mesh.FlipZ();
	}

	mesh.ComputeBounds();
	mesh.ComputeMeshletData(this->GetAllocator());

	mesh.BuildRenderData(this->GetAllocator(), gfxDevice);

	return retVal;
}
#else

Entity PrefabFactory::CreateCube(
	rhi::GfxDevice* gfxDevice,
	entt::entity matId,
	float size,
	bool rhsCoord)
{}

Entity PrefabFactory::CreateSphere(
	rhi::GfxDevice* gfxDevice,
	entt::entity matId,
	float diameter,
	size_t tessellation,
	bool rhcoords)
{}

Entity PrefabFactory::CreatePlane(
	rhi::GfxDevice* gfxDevice,
	entt::entity matId,
	float width,
	float height,
	bool rhCoords)
{}
#endif