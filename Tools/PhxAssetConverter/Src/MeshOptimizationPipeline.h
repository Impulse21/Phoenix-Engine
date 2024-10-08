#pragma once

#include <PhxEngine/Core/Memory.h>
#include "PipelineTypes.h"
#include <functional>

namespace PhxEngine::Pipeline
{
	class MeshOptimizationPipeline
	{
	public:
		MeshOptimizationPipeline() = default;

		void Optimize(PhxEngine::StackAllocator& tempAllocator, Mesh& mesh) const;

	private:
		void OptmizeInternal(PhxEngine::StackAllocator& tempAllocator, Mesh& mesh) const;
	};
}

