#pragma once

#include <PhxEngine/App/Layer.h>
#include <PhxEngine/App/Application.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <DirectXMath.h>

class SceneRenderLayer : public PhxEngine::AppLayer
{
public:
	SceneRenderLayer();

	void OnAttach() override;
	void OnRender() override;

	PhxEngine::RHI::TextureHandle GetFinalColourBuffer()
	{
		return this->m_colourBuffers[PhxEngine::LayeredApplication::Ptr->GetFrameCount() % this->m_colourBuffers.size()];
	}

	void ResizeSurface(DirectX::XMFLOAT2 const& size);

private:
	void CreateWindowTextures(DirectX::XMFLOAT2 const& size);

private:
	std::vector<PhxEngine::RHI::TextureHandle> m_colourBuffers;
	PhxEngine::RHI::CommandListHandle m_commandList;
};

