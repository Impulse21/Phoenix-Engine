#pragma once

namespace PhxEngine
{
	// Forward Dec
	namespace RHI
	{
		class IGraphicsDevice;
	}

	namespace Renderer
	{
		class RenderSystem;
	}

	struct EngineEnvironment
	{
		RHI::IGraphicsDevice* pGraphicsDevice = nullptr;
		Renderer::RenderSystem* pRenderSystem = nullptr;
	};
}