#pragma once

#include <PhxEngine/Core/Primitives.h>
#include <PhxEngine/Core/UUID.h>

#include <PhxEngine/RHI/PhxRHI.h>

#include <PhxEngine/Shaders/ShaderInteropStructures_NEW.h>

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
		Shader::New::Scene GpuData;

	
		// GPU Data
		RHI::BufferHandle ObjectInstancesBuffer;

		RHI::BufferHandle GeometryBuffer;
		RHI::BufferHandle GeometryBoundsBuffer;
		RHI::BufferHandle MaterialBuffer;

		// Only required for non mesh shaders...
		RHI::BufferHandle GlobalIndexBuffer;

		RHI::BufferHandle MeshletsBuffer;
		RHI::BufferHandle VertexIBBuffer;
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

		void Initialize(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene);
		void PrepareFrame(RHI::IGraphicsDevice* gfxDevice, RHI::ICommandList* commandList, Scene::Scene& scene);

		void Update(Core::TimeStep const& deltaTime);
		void UploadGpuData();

	private:
		Renderer::ResourceUpload UpdateMaterialData(RHI::IGraphicsDevice* gfxDevice, Scene::Scene& scene);
		Renderer::ResourceUpload UpdateGeometryData(RHI::IGraphicsDevice* gfxDevice, Scene::Scene& scene);
	};
}
