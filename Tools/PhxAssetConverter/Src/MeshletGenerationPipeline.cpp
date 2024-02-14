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
		this->GenerateMeshletData(meshPart);
		this->GenerateMeshletBounds(meshPart);
	}
	Core::TimeStep timeStep = stopwatch.Elapsed();
	PHX_LOG_INFO("Finished Generating Meshlet Data for %s [%f seconds]", mesh.Name.c_str(), timeStep.GetSeconds());
}

void PhxEngine::Pipeline::MeshletGenerationPipeline::GenerateMeshletData(MeshPart& meshpart) const
{
	size_t maxMeshlets = meshopt_buildMeshletsBound(meshpart.Indices.size(), this->m_maxVertices, this->m_maxTriangles);
	meshpart.Meshlets.resize(maxMeshlets);
	meshpart.MeshletVertices.resize(maxMeshlets * this->m_maxVertices);
	meshpart.MeshletTriangles.resize(maxMeshlets * this->m_maxTriangles * 3);

	size_t meshletCount = meshopt_buildMeshlets(
		meshpart.Meshlets.data(),
		meshpart.MeshletVertices.data(),
		meshpart.MeshletTriangles.data(),
		meshpart.Indices.data(),
		meshpart.Indices.size(),
		meshpart.GetStream(VertexStreamType::Position).Data.data(),
		meshpart.GetNumVertices(),
		meshpart.GetStream(VertexStreamType::Position).GetElementStride(),
		this->m_maxVertices,
		this->m_maxTriangles,
		this->m_coneWeight);

	// Trime the data;
	const meshopt_Meshlet& last = meshpart.Meshlets[meshletCount - 1];

	meshpart.MeshletVertices.resize(last.vertex_offset + last.vertex_count);
	meshpart.MeshletTriangles.resize(last.triangle_offset + ((last.triangle_count * 3 + 3) & ~3));
	meshpart.Meshlets.resize(meshletCount);
}

void PhxEngine::Pipeline::MeshletGenerationPipeline::GenerateMeshletBounds(MeshPart& meshpart) const
{
	meshpart.MeshletBounds.resize(meshpart.Meshlets.size());
	for (size_t m = 0; m < meshpart.Meshlets.size(); m++)
	{
		auto& meshlet = meshpart.Meshlets[m];
		meshpart.MeshletBounds[m] =
			meshopt_computeMeshletBounds(
				&meshpart.MeshletVertices[meshlet.vertex_offset],
				&meshpart.MeshletTriangles[meshlet.triangle_offset],
				meshlet.triangle_count,
				meshpart.GetStream(VertexStreamType::Position).Data.data(),
				meshpart.GetNumVertices(),
				meshpart.GetStream(VertexStreamType::Position).GetElementStride());
	}
}
