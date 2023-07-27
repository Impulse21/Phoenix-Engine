#pragma once

namespace PhxEngine::Renderer
{
	struct RendererParameters
	{
		bool EnableImgui = true;
	};

	void Initialize(RendererParameters const& parameters);
	void Finalize();

	void BeginFrame();
	void Present();

}