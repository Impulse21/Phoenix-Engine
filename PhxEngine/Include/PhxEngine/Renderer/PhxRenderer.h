#pragma once

namespace PhxEngine::Renderer
{
	bool Initialize();
	bool Finialize();

	// Global Single Interface to the renderer
	class RendererModule
	{
	public:
		inline static RendererModule* Impl;

	public:
		RendererModule() = default;

	};
}

