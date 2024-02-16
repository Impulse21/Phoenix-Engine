#include "MeshletGenerationPipeline.h"

#include <PhxEngine/Core/Log.h>
#include <PhxEngine/Core/StopWatch.h>
#include <meshoptimizer.h>

void PhxEngine::Pipeline::MeshletGenerationPipeline::GenerateMeshletData(Mesh& mesh) const
{
	PHX_LOG_INFO("Generating Meshlet Data for %s", mesh.Name.c_str());

	Core::StopWatch stopwatch;
	for (auto& meshPart : mesh.MeshParts)
	{
		this->GenerateMeshletData(mesh, meshPart);
		this->GenerateMeshletBounds(mesh, meshPart);
	}
	Core::TimeStep timeStep = stopwatch.Elapsed();
	PHX_LOG_INFO("Finished Generating Meshlet Data for %s [%f seconds]", mesh.Name.c_str(), timeStep.GetSeconds());
}

void PhxEngine::Pipeline::MeshletGenerationPipeline::GenerateMeshletData(Mesh& mesh, MeshPart& meshPart) const
{
	const size_t indexCount = meshPart.IndexCount;
	const size_t indexOffset = meshPart.IndexOffset;
	size_t maxMeshlets = meshopt_buildMeshletsBound(indexCount, this->m_maxVertices, this->m_maxTriangles);
	meshPart.Meshlets.resize(maxMeshlets);
	meshPart.MeshletVertices.resize(maxMeshlets * this->m_maxVertices);
	meshPart.MeshletTriangles.resize(maxMeshlets * this->m_maxTriangles * 3);

	size_t meshletCount = meshopt_buildMeshlets(
		meshPart.Meshlets.data(),
		meshPart.MeshletVertices.data(),
		meshPart.MeshletTriangles.data(),
		mesh.Indices.data() + indexOffset,
		indexCount,
		mesh.GetStream(VertexStreamType::Position).Data.data(),
		mesh.GetNumVertices(),
		mesh.GetStream(VertexStreamType::Position).GetElementStride(),
		this->m_maxVertices,
		this->m_maxTriangles,
		this->m_coneWeight);

	// Trime the data;
	const meshopt_Meshlet& last = meshPart.Meshlets[meshletCount - 1];

	meshPart.MeshletVertices.resize(last.vertex_offset + last.vertex_count);
	meshPart.MeshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
	meshPart.Meshlets.resize(meshletCount);
}

void PhxEngine::Pipeline::MeshletGenerationPipeline::GenerateMeshletBounds(Mesh& mesh, MeshPart& meshPart) const
{
	meshPart.MeshletBounds.resize(meshPart.Meshlets.size());
	for (size_t m = 0; m < meshPart.Meshlets.size(); m++)
	{
		auto& meshlet = meshPart.Meshlets[m];
		meshPart.MeshletBounds[m] =
			meshopt_computeMeshletBounds(
				&meshPart.MeshletVertices[meshlet.vertex_offset],
				&meshPart.MeshletTriangles[meshlet.triangle_offset],
				meshlet.triangle_count,
				mesh.GetStream(VertexStreamType::Position).Data.data(),
				mesh.GetNumVertices(),
				mesh.GetStream(VertexStreamType::Position).GetElementStride());
	}
}
