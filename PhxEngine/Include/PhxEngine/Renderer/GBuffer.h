#pragma once

#include <PhxEngine/RHI/PhxRHI.h>
#include <DirectXMath.h>

namespace PhxEngine::Renderer
{
	struct GBufferRenderTargets
	{
        RHI::TextureHandle DepthTex;
        RHI::TextureHandle AlbedoTex;
        RHI::TextureHandle NormalTex;
        RHI::TextureHandle SurfaceTex;
        RHI::TextureHandle SpecularTex;
        RHI::TextureHandle EmissiveTex;

        RHI::RenderPassHandle RenderPass;
        DirectX::XMFLOAT2 CanvasSize;

        std::vector<RHI::RHIFormat> GBufferFormats;
        RHI::RHIFormat DepthFormat;

        virtual void Initialize(
            RHI::IGraphicsDevice* gfxDevice,
            DirectX::XMFLOAT2 const& size);

        virtual void Free(RHI::IGraphicsDevice* gfxDevice);
	};
}