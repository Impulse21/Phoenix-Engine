#include "MeshOptimizationPipeline.h"

#include <meshoptimizer.h>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Core/Log.h>

using namespace PhxEngine;


void PhxEngine::Pipeline::MeshOptimizationPipeline::Optimize(Core::LinearAllocator& tempAllocator, Mesh& mesh) const
{
	PHX_LOG_INFO("Optimizing Mesh %s", mesh.Name.c_str());

	Core::StopWatch stopwatch;
	for (auto& meshPart : mesh.MeshParts)
	{
		tempAllocator.Clear();
		this->OptimizeMeshPart(tempAllocator, meshPart);
	}
	Core::TimeStep timeStep = stopwatch.Elapsed();
	PHX_LOG_INFO("Finished optimizng Mesh %s [%f seconds]", mesh.Name.c_str(), timeStep.GetSeconds());
}

void PhxEngine::Pipeline::MeshOptimizationPipeline::OptimizeMeshPart(Core::LinearAllocator& tempAllocator, Pipeline::MeshPart& meshPart) const
{
	// Copy vertex data and preocess
	Core::StopWatch stopWatch;

	float* unindexedStreams[static_cast<size_t>(Pipeline::VertexStreamType::NumStreams)];
	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = meshPart.VertexStreams[iStream];
		unindexedStreams[iStream] = tempAllocator.AllocateArray<float>(stream.Data.size(), 1);
		std::memcpy(unindexedStreams[iStream], stream, sizeof(float) * stream.Data.size());
	}

	uint32_t* originalIndices = nullptr;
	uint32_t totalIndices = meshPart.GetNumVertices();
	if (meshPart.HasIndices())
	{
		originalIndices = tempAllocator.AllocateArray<uint32_t>(meshPart.Indices.size(), 1);
		totalIndices = meshPart.Indices.size();
		std::memcpy(originalIndices, meshPart.Indices.data(), sizeof(uint32_t) * totalIndices);
	}

	uint32_t* remap = tempAllocator.AllocateArray<uint32_t>(totalIndices, 1);
	meshopt_Stream meshOptStreams[static_cast<size_t>(Pipeline::VertexStreamType::NumStreams)];

	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = meshPart.VertexStreams[iStream];
		meshOptStreams[iStream] = { unindexedStreams[iStream], sizeof(float) * stream.NumComponents, sizeof(float) * stream.NumComponents };
	}

	size_t totalVertices = meshopt_generateVertexRemapMulti(remap, originalIndices, totalIndices, totalIndices, meshOptStreams, sizeof(meshOptStreams) / sizeof(meshOptStreams[0]));

	meshPart.Indices.resize(totalIndices);
	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = meshPart.VertexStreams[iStream];
		stream.Data.resize(totalVertices * stream.NumComponents);
	}

	meshopt_remapIndexBuffer(meshPart.Indices.data(), originalIndices, totalIndices, remap);
	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = meshPart.VertexStreams[iStream];
		meshopt_remapVertexBuffer(stream.Data.data(), unindexedStreams[iStream], totalIndices, sizeof(float) * stream.NumComponents, remap);
	}

	Core::TimeStep reindexTimeSpan = stopWatch.Elapsed();

	stopWatch.Begin();

	meshopt_optimizeVertexCache(meshPart.Indices.data(), meshPart.Indices.data(), meshPart.Indices.size(), meshPart.GetNumVertices());
	// meshopt_optimizeOverdraw(meshPart.Indices.data(), meshPart.Indices.data(), meshPart.Indices.size(), &vertices[0].x, vertex_count, sizeof(Vertex), 1.05f);
	meshopt_optimizeVertexFetchRemap(remap, meshPart.Indices.data(), meshPart.Indices.size(), totalVertices);
	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = meshPart.VertexStreams[iStream];
		meshopt_remapVertexBuffer(stream.Data.data(), unindexedStreams[iStream], totalIndices, sizeof(float) * stream.NumComponents, remap);
	}

	Core::TimeStep optimizeTimestep = stopWatch.Elapsed();
	PHX_LOG_INFO("\tProcessing Mesh Part took %f reindexing, %f optmization", reindexTimeSpan.GetSeconds(), optimizeTimestep.GetSeconds());
}
