#pragma once

#include <PhxEngine/Core/FlexArray.h>
#include <PhxEngine/Core/Memory.h>
#include "PipelineTypes.h"
#include <functional>

namespace PhxEngine::Pipeline
{
	class MeshOptimizationPipeline
	{
	public:
		MeshOptimizationPipeline() = default;

		void Optimize(Core::LinearAllocator& tempAllocator, Mesh& mesh) const;

	private:
		void OptmizeInternal(Core::LinearAllocator& tempAllocator, Mesh& mesh) const;
	};
}

