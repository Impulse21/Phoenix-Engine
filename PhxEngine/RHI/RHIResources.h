#pragma once

#include <assert.h>
// -- Switches based on backend ---
#include <PlatformTypes.h>
#include <RHI/RHIEnums.h>

namespace PhxEngine::RHI
{
	// forward delclare for friend factory
	class Factory;

	struct Color
	{
		float R;
		float G;
		float B;
		float A;

		Color()
			: R(0.f), G(0.f), B(0.f), A(0.f)
		{ }

		Color(float c)
			: R(c), G(c), B(c), A(c)
		{ }

		Color(float r, float g, float b, float a)
			: R(r), G(g), B(b), A(a) { }

		bool operator ==(const Color& other) const { return R == other.R && G == other.G && B == other.B && A == other.A; }
		bool operator !=(const Color& other) const { return !(*this == other); }
	};

	union ClearValue
	{
		// TODO: Change to be a flat array
		// float Colour[4];
		Color Colour;
		struct ClearDepthStencil
		{
			float Depth;
			uint32_t Stencil;
		} DepthStencil;
	};

	struct NonCopyable
	{
		NonCopyable() = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};

	struct PlatformContext {}; // TODO:

	class GfxContext;
	class ComputeContext;
	class CopyContext;
	class CommandContext : NonCopyable
	{
	public:
		static CommandContext& Begin();

		GfxContext& GetGraphicsContext()
		{
			assert(this->m_type == CommandContextType::Graphics, "Cannot convert to graphics");
			return reinterpret_cast<GfxContext&>(*this);
		}

		ComputeContext& GetComputeContext()
		{
			assert(this->m_type == CommandContextType::Compute, "Cannot convert to compute");
			return reinterpret_cast<ComputeContext&>(*this);
		}

		CopyContext& GetCopyContext()
		{
			assert(this->m_type == CommandContextType::Copy, "Cannot convert to compute");
			return reinterpret_cast<CopyContext&>(*this);
		}

	public:
		// TODO: Interface functions

	private:
		CommandContext(CommandContextType type)
			: m_type(type)
		{
		}

	protected:
		PlatformContext m_platformContext;
		CommandContextType m_type;
	};

	class GfxContext : public CommandContext
	{
	public:

		static GfxContext& Begin()
		{
			return CommandContext::Begin().GetGraphicsContext();
		}
	};

	class ComputeContext : public CommandContext
	{
	public:

		static GfxContext& Begin()
		{
			return CommandContext::Begin().GetGraphicsContext();
		}
	};

	class CopyContext : public CommandContext
	{
	public:

		static GfxContext& Begin()
		{
			return CommandContext::Begin().GetGraphicsContext();
		}
	};

	struct GpuBufferDesc
	{

	};
	class GpuBuffer
	{
		friend Factory;
	public:
	private:
		PlatformGpuBuffer m_platformImpl;
	};

	struct TextureDesc
	{

	};

	class Texture
	{
		friend Factory;
	public:
	private:
	};

	class SubmitRecipt
	{
	private:
		PlatformSubmitRecipt m_platformImpl;
	};

	struct GfxPipelineDesc
	{

	};
	class GfxPipeline
	{
		friend Factory;
	private:
	};

	struct ComputePipelineDesc
	{

	};
	class ComputePipeline
	{
		friend Factory;
	private:
	};

	struct MeshPipelineDesc
	{

	};
	class MeshPipeline
	{
		friend Factory;
	private:
	};

	struct InputLayoutDesc
	{

	};
	class InputLayout
	{
		friend Factory;
	private:
	};

	struct ShaderDesc
	{

	};
	class Shader
	{
		friend Factory;
	private:
	};

	struct CommandSignatureDesc
	{
		friend Factory;

	};
	class CommandSignature
	{
		friend Factory;

	};

	struct SwapchainDesc
	{
		uint32_t Width = 1u;
		uint32_t Height = 1u;
		uint32_t BufferCount = 3;
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

	class SwapChain
	{
		friend Factory;
	public:

	private:
		SwapchainDesc m_desc;
	};

}