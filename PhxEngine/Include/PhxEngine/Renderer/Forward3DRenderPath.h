#pragma once

#include <array>
#include <PhxEngine/RHI/PhxRHI.h>

namespace tf
{
	class Task;
	class Taskflow;
}

namespace PhxEngine::Graphics
{
	class ShaderFactory;
}

namespace PhxEngine::Renderer
{
	class CommonPasses;
	enum class EGfxPipelineStates
	{
		Count
	};

	enum class EComputePipelineStates
	{
		Count
	};

	enum class EMeshPipelineStates
	{
		Count
	};

	class Forward3DRenderPath
	{
	public:
		Forward3DRenderPath(
			RHI::IGraphicsDevice* gfxDevice,
			std::shared_ptr<Renderer::CommonPasses> commonPasses,
			std::shared_ptr<Graphics::ShaderFactory> shaderFactory);

		void Initialize();

	private:
		tf::Task LoadShaders(tf::Taskflow& taskFlow);
		tf::Task LoadPipelineStates(tf::Taskflow& taskFlow);

	private:
		RHI::IGraphicsDevice* m_gfxDevice;
		std::shared_ptr<Graphics::ShaderFactory> m_shaderFactory;
		std::shared_ptr<Renderer::CommonPasses> m_commonPasses;

		std::array<RHI::GraphicsPipelineHandle, static_cast<size_t>(EGfxPipelineStates::Count)> m_gfxStates;
		std::array<RHI::GraphicsPipelineHandle, static_cast<size_t>(EComputePipelineStates::Count)> m_computeStates;
		std::array<RHI::GraphicsPipelineHandle, static_cast<size_t>(EMeshPipelineStates::Count)> m_meshStates;
	};
}

