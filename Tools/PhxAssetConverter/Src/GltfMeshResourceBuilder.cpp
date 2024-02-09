#include "ResourceBuilders.h"

using namespace PhxEngine;
#include <vector>
#include <meshoptimizer.h>

MeshResource::Header* GltfMeshResourceBuilder::Build(cgltf_mesh const& src)
{
	// Allocate the header, then process.
	this->m_meshOptmizationPipeline->Optimize();
	this->m_meshOptmizationPipeline->BuildMeshlets();

	return nullptr;
}

void MeshOptimizationPipeline::Optimize()
{
	this->ComputeIndexing();
	
	if (false)
	{
		this->Simplification();
	}
	this->VertexCacheOptmization();
	this->OverdrawOptimization();
	this->VertexFetchOptmization();
	this->VertexQuantization();

	if (false)
	{
		this->BufferCompression();
	}
}

void MeshOptimizationPipeline::BuildMeshlets()
{

}


void MeshOptimizationPipeline::ComputeIndexing()
{
	const size_t faceCount = 3;
	size_t indexCount = faceCount * 3;
	size_t unindexedVertexCount = faceCount * 3;
	std::vector<unsigned int> remap(indexCount); // allocate temporary memory for the remap table
	size_t vertex_count = meshopt_generateVertexRemap(&remap[0], NULL, indexCount, &unindexed_vertices[0], unindexedVertexCount, sizeof(Vertex));
	meshopt_generateVertexRemapMulti()
	meshopt_remapIndexBuffer(indices, NULL, indexCount, &remap[0]);
	meshopt_remapVertexBuffer(vertices, &unindexed_vertices[0], unindexedVertexCount, sizeof(Vertex), &remap[0]);


}

void MeshOptimizationPipeline::Simplification()
{
	//no-op
}

void MeshOptimizationPipeline::VertexCacheOptmization()
{

}

void MeshOptimizationPipeline::OverdrawOptimization()
{

}

void MeshOptimizationPipeline::VertexFetchOptmization()
{

}

void MeshOptimizationPipeline::VertexQuantization()
{

}

void MeshOptimizationPipeline::BufferCompression()
{
	//no-op
}
