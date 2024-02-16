#pragma once

#include "PipelineTypes.h"

namespace PhxEngine::Pipeline
{
	class MeshletGenerationPipeline
	{
	public:
		MeshletGenerationPipeline(size_t maxVertices, size_t maxTriangles, float coneWeight = 0.0f)
			: m_maxVertices(maxVertices)
			, m_maxTriangles(maxTriangles)
			, m_coneWeight(coneWeight)
		{}

		void GenerateMeshletData(Mesh& mesh) const;

	private:
		void GenerateMeshletData(Mesh& mesh, MeshPart& meshpart) const;
		void GenerateMeshletBounds(Mesh& mesh, MeshPart& meshpart) const;

	private:
		const size_t m_maxVertices;
		const size_t m_maxTriangles;
		const size_t m_coneWeight;

	};
}

