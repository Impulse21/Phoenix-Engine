#pragma once

#include <string>
#include <cgltf.h>

#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/Memory.h>
#include <PhxEngine/Resource/ResourceFormats.h>
#include <PhxEngine/Core/EnumClassFlags.h>

namespace PhxEngine
{
	class ResourceBuilder
	{
	public:
		virtual ~ResourceBuilder() = default;

	protected:
		ResourceBuilder() = default;
		ResourceBuilder(PhxEngine::Core::IAllocator* allocator)
			: m_allocator(allocator) {};

		PhxEngine::Core::IAllocator* m_allocator;
	};

	class MeshOptimizationPipeline
	{
	public:
		void Optimize();
		void BuildMeshlets();

	private:
		void ComputeIndexing();
		void Simplification();
		void VertexCacheOptmization();
		void OverdrawOptimization();
		void VertexFetchOptmization();
		void VertexQuantization();
		void BufferCompression();
	};

	class GltfMeshImporter
	{
	public:

		MeshResource::Header* Build(cgltf_mesh const& src);
	};
	class GltfMeshResourceBuilder : public ResourceBuilder
	{
	public:
		GltfMeshResourceBuilder(PhxEngine::Core::IAllocator* allocator, MeshOptimizationPipeline* optmizationPipeline)
			: ResourceBuilder(allocator) 
			, m_meshOptmizationPipeline(optmizationPipeline) {};

		MeshResource::Header* Build(cgltf_mesh const& src);

	private:
		void MeshOptimizePipeline();
		void GenerateMeshlets();

	private:
		MeshOptimizationPipeline* m_meshOptmizationPipeline;
	};

	class GltfMaterialBuilder : public ResourceBuilder
	{
	public:
		MaterialResource::Header* Build(cgltf_material const& src) { return nullptr; }
	};

	class GltfTextureBuilder : public ResourceBuilder
	{
	public:
		TextureResource::Header* Build(cgltf_texture const& src) { return nullptr; }
	};
}

