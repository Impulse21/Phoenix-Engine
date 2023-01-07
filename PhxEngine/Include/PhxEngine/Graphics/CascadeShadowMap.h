#pragma once

#include <stdint.h>
#include <PhxEngine/RHI/PhxRHI.h>
#include <PhxEngine/Renderer/Renderer.h>
#include <PhxEngine/Scene/Components.h>

namespace PhxEngine::Graphics
{
	class CascadeShadowMap
	{
	public:
		CascadeShadowMap(uint32_t resolution, RHI::FormatType format, bool isReverseZ);
		~CascadeShadowMap();

		// Construct Shadow Render Cams
		// TODO: Return Shadow came
		std::vector<Renderer::RenderCam> CreateRenderCams(
			Scene::CameraComponent const& cameraComponent,
			Scene::LightComponent& lightComponent,
			float maxZDepth);

		constexpr static size_t GetNumCascades() { return kNumCascades; }
		PhxEngine::RHI::RenderPassHandle GetRenderPass() const { return this->m_renderPass; }

		RHI::DescriptorIndex GetTextureArrayIndex() { return RHI::IGraphicsDevice::Ptr->GetDescriptorIndex(this->m_shadowMapTexArray, RHI::SubresouceType::SRV); }

	private:
		const bool m_isReverseZ;
		constexpr static size_t kNumCascades = 3;
		RHI::TextureHandle m_shadowMapTexArray;
		PhxEngine::RHI::RenderPassHandle m_renderPass;
	};
}

