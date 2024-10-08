#include "MeshOptimizationPipeline.h"

#include <meshoptimizer.h>
#include <PhxEngine/Core/StopWatch.h>
#include <PhxEngine/Core/Logger.h>

using namespace PhxEngine;


void PhxEngine::Pipeline::MeshOptimizationPipeline::Optimize(StackAllocator& tempAllocator, Mesh& mesh) const
{
	PHX_LOG_INFO("Optimizing Mesh %s", mesh.Name.c_str());

	StopWatch stopwatch;

	this->OptmizeInternal(tempAllocator, mesh);

	TimeStep timeStep = stopwatch.Elapsed();
	PHX_LOG_INFO("Finished optimizng Mesh %s [%f seconds]", mesh.Name.c_str(), timeStep.GetSeconds());
}

void PhxEngine::Pipeline::MeshOptimizationPipeline::OptmizeInternal(StackAllocator& tempAllocator, Mesh& mesh) const
{
	// Copy vertex data and preocess
	StopWatch stopWatch;

	// -- Make a copy of the original indices ---
	float* unindexedStreams[static_cast<size_t>(Pipeline::VertexStreamType::NumStreams)];
	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = mesh.VertexStreams[iStream];
		if (!stream.IsEmpty())
		{
			unindexedStreams[iStream] = tempAllocator.NewArr<float>(stream.Data.size(), 1);
			std::memcpy(unindexedStreams[iStream], stream, sizeof(float) * stream.Data.size());
		}
	}

	uint32_t* originalIndices = nullptr;
	uint32_t totalIndices = mesh.GetNumVertices();
	if (mesh.HasIndices())
	{
		originalIndices = tempAllocator.NewArr<uint32_t>(mesh.Indices.size(), 1);
		totalIndices = mesh.Indices.size();
		std::memcpy(originalIndices, mesh.Indices.data(), sizeof(uint32_t) * totalIndices);
	}
	else
	{
		mesh.Indices.resize(totalIndices);
	}

	uint32_t* remap = tempAllocator.NewArr<uint32_t>(totalIndices, 1);
	std::vector<meshopt_Stream> meshOptStreams;
	meshOptStreams.reserve(static_cast<size_t>(Pipeline::VertexStreamType::NumStreams));

	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = mesh.VertexStreams[iStream];
		if (!stream.IsEmpty())
			meshOptStreams.push_back({ unindexedStreams[iStream], sizeof(float) * stream.NumComponents, sizeof(float) * stream.NumComponents });
	}

	size_t totalVertices = meshopt_generateVertexRemapMulti(remap, originalIndices, totalIndices, totalIndices, meshOptStreams.data(), meshOptStreams.size());

	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = mesh.VertexStreams[iStream];
		if (!stream.IsEmpty())
			stream.Data.resize(totalVertices * stream.NumComponents);
	}

	meshopt_remapIndexBuffer(mesh.Indices.data(), originalIndices, totalIndices, remap);
	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = mesh.VertexStreams[iStream];
		if (!stream.IsEmpty())
			meshopt_remapVertexBuffer(stream.Data.data(), unindexedStreams[iStream], totalIndices, sizeof(float) * stream.NumComponents, remap);
	}

	TimeStep reindexTimeSpan = stopWatch.Elapsed();

	stopWatch.Begin();

	meshopt_optimizeVertexCache(mesh.Indices.data(), mesh.Indices.data(), mesh.Indices.size(), mesh.GetNumVertices());
	// meshopt_optimizeOverdraw(meshPart.Indices.data(), meshPart.Indices.data(), meshPart.Indices.size(), &vertices[0].x, vertex_count, sizeof(Vertex), 1.05f);
	meshopt_optimizeVertexFetchRemap(remap, mesh.Indices.data(), mesh.Indices.size(), totalVertices);
	for (size_t iStream = 0; iStream < static_cast<size_t>(Pipeline::VertexStreamType::NumStreams); iStream++)
	{
		Pipeline::VertexStream& stream = mesh.VertexStreams[iStream];
		if (!stream.IsEmpty())
			meshopt_remapVertexBuffer(stream.Data.data(), unindexedStreams[iStream], totalIndices, sizeof(float) * stream.NumComponents, remap);
	}

	TimeStep optimizeTimestep = stopWatch.Elapsed();
	PHX_LOG_INFO("\tProcessing Mesh Part took %f reindexing, %f optmization", reindexTimeSpan.GetSeconds(), optimizeTimestep.GetSeconds());
}
