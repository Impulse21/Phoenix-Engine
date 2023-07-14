#pragma once

#include <PhxEngine/Core/Window.h>
#include <PhxEngine/Renderer/RendererTypes.h>

namespace PhxEngine::Renderer
{
	bool Initialize();
	bool Finialize();

	// Global Single Interface to the renderer
	class IRenderService
	{
	public:
		inline static IRenderService* Impl;

	public:
		virtual ~IRenderService() = default;

		virtual void Render() = 0;

		// Factory methods
		virtual void CreateSwapChain() = 0;
		virtual void RegisterViewport() = 0;
		virtual DrawableHandle CreateDrawable(MeshDesc const& meshDesc) = 0;
		virtual RenderMaterialHandle CreateMaterial(MaterialDesc const& materialDesc) = 0;
		virtual DrawableInstaceHandle RegisterDrawableInstance(DrawableInstanceDesc const& instanceData) = 0;

	};
}

