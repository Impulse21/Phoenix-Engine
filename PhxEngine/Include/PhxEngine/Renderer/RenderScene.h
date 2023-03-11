#pragma once

#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/Core/UUID.h>

#include <PhxEngine/RHI/PhxRHI.h>

#include <PhxEngine/Shaders/ShaderInteropStructures.h>

#include <DirectXMesh.h>

namespace PhxEngine::Scene
{
	class Scene;
}

namespace PhxEngine::Renderer
{
	// Render Scene Components

	class Entity;

	struct RenderScene
	{
		Core::AABB Bounds;
		Shader::Scene GpuData;

		std::vector<Shader::Meshlet_NEW> Meshlets;
		std::vector<DirectX::MeshletTriangle> MeshletTriangles;
		std::vector<uint8_t> UniqueVertexIB;
		std::vector<Shader::MeshletVertexPositions> MeshletPositions;
		std::vector<Shader::MeshletPackedVertexData> MeshletVertexData;

		std::vector<DirectX::CullData> MeshletCullData;

		// GPU Data
		RHI::BufferHandle ObjectInstancesBuffer;

		RHI::BufferHandle GeometryBuffer;
		RHI::BufferHandle GeometryBoundsBuffer;
		RHI::BufferHandle MaterialBuffer;

		// Only required for non mesh shaders...
		RHI::BufferHandle GlobalIndexBuffer;

		RHI::BufferHandle MeshletsBuffer;
		RHI::BufferHandle MeshletsBuffer;
		RHI::BufferHandle MeshletTrianglesBuffer;

		RHI::CommandSignatureHandle IndirectDrawCommandSignature;

		RHI::BufferHandle IndirectDrawCulled;
		RHI::BufferHandle IndirectDrawEarly;
		RHI::BufferHandle IndirectDrawLate;

		struct Settings
		{
			bool UseMeshlets;
		};

		// GPU Buffers

		void Initialize(Scene::Scene& scene);

		void UploadGpuData();

		void PrepareGpuData();
		void Update();

	};
}

inline float4 GetColour()
{
