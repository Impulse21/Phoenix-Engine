#pragma once

#include <assert.h>
#include <string>
#include <variant>
#include <optional>

#include <Core/Span.h>
#include <Core/Handle.h>
#include <Core/NonCopyable.h>
#include <RHI/RHIEnums.h>
#include <RHI/RHIDefinitions.h>

// -- Exposes Internal Platform Type alias Trick to avoid virtuals ---
#include <PlatformTypes.h>


namespace PhxEngine::RHI
{
	class RHIResource : Core::NonCopyable
	{
	public:
		RHIResource() = default;
	};

	class CommandList
	{
	public:
		
	};

	struct TextureDesc
	{
		TextureMiscFlags MiscFlags = TextureMiscFlags::None;
		BindingFlags BindingFlags = BindingFlags::ShaderResource;
		TextureDimension Dimension = TextureDimension::Texture2D;
		ResourceStates InitialState = ResourceStates::Common;
		RHI::Format Format = RHI::Format::UNKNOWN;

		uint32_t Width;
		uint32_t Height;

		union
		{
			uint16_t ArraySize = 1;
			uint16_t Depth;
		};

		uint16_t MipLevels = 1;

		RHI::ClearValue OptmizedClearValue = {};
		std::string DebugName;
	};

	class Texture final : public RHIResource
	{
		friend RHI::Factory;

	public:
		Texture() = default;

		const TextureDesc& const Desc() const { return this->m_Desc; }
		const PlatformTexture& Platform() const { return this->m_PlatfomrTexture; }
		PlatformTexture& Platform() { return this->m_PlatfomrTexture; }

	private:
		TextureDesc m_Desc;
		PlatformTexture m_PlatfomrTexture;
	};

	struct SwapChainDesc
	{
		uint32_t Width = 1u;
		uint32_t Height = 1u;
		RHI::Format Format = RHI::Format::R10G10B10A2_UNORM;
		bool Fullscreen = false;
		bool VSync = false;
		bool EnableHDR = false;
		RHI::ClearValue OptmizedClearValue =
		{
			.Colour =
			{
				0.0f,
				0.0f,
				0.0f,
				1.0f,
			}
		};
	};

	class SwapChain final : public RHIResource
	{
		friend RHI::Factory;

	public:
		SwapChain() = default;

		const SwapChainDesc& const Desc() const { return this->m_Desc; }
		const PlatformSwapChain& Platform() const { return this->m_PlatfomrSwapChain; }
		PlatformSwapChain& Platform() { return this->m_PlatfomrSwapChain; }

	private:
		SwapChainDesc m_Desc;
		PlatformSwapChain m_PlatfomrSwapChain;
	};
}