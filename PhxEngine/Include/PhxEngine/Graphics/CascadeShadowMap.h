#pragma once

#include <stdint.h>
#include <PhxEngine/Graphics/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Scene/Components.h>

namespace PhxEngine::Graphics
{
	class CascadeShadowMap
	{
	public:
		CascadeShadowMap(uint32_t resolution, uint16_t numCascades, RHI::FormatType format, bool isReverseZ);
		~CascadeShadowMap();

		// Construct Shadow Render Cams
		// TODO: Return Shadow came
		std::vector<Renderer::RenderCam> CreateRenderCams(
			Scene::CameraComponent const& cameraComponent,
			Scene::LightComponent& lightComponent,
			uint32_t maxZDepth);

		uint16_t GetNumCascades() const { return this->m_numCascades; }
		PhxEngine::RHI::RenderPassHandle GetRenderPass() const { return this->m_renderPass; }

		RHI::DescriptorIndex GetTextureArrayIndex() { return RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_shadowMapTexArray, RHI::SubresouceType::SRV); }

	private:
		const bool m_isReverseZ;
		const uint16_t m_numCascades;
		RHI::TextureHandle m_shadowMapTexArray;
		PhxEngine::RHI::RenderPassHandle m_renderPass;
	};
}

