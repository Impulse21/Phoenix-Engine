#pragma once

#include <assert.h>
#include <Core/RefCountPtr.h>

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
	class GpuBuffer final
	{
		friend Factory;

	public:
		const GpuBufferDesc& GetDesc() const { return this->m_desc; };
		const PlatformGpuBuffer& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformGpuBuffer m_platformImpl;
		GpuBufferDesc m_desc;
	};

	struct TextureDesc
	{

	};

	class Texture final
	{
		friend Factory;

	public:
		const SwapchainDesc& GetDesc() const { return this->m_desc; };
		const PlatformSwapChain& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformSwapChain m_platformImpl;
		SwapchainDesc m_desc;
	};

	class SubmitRecipt
	{
	private:
		PlatformSubmitRecipt m_platformImpl;
	};

	struct GfxPipelineDesc
	{

	};
	class GfxPipeline final
	{
		friend Factory;

	public:
		const GfxPipelineDesc& GetDesc() const { return this->m_desc; };
		const PlatformGfxPipeline& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformGfxPipeline m_platformImpl;
		GfxPipelineDesc m_desc;
	};

	struct ComputePipelineDesc
	{

	};
	class ComputePipeline final
	{
		friend Factory;

	public:
		const ComputePipelineDesc& GetDesc() const { return this->m_desc; };
		const PlatformComputePipeline& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformComputePipeline m_platformImpl;
		ComputePipelineDesc m_desc;
	};

	struct MeshPipelineDesc
	{

	};
	class MeshPipeline final
	{
		friend Factory;

	public:
		const MeshPipelineDesc& GetDesc() const { return this->m_desc; };
		const PlatformMeshPipeline& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformMeshPipeline m_platformImpl;
		MeshPipelineDesc m_desc;
	};

	struct InputLayoutDesc
	{

	};
	class InputLayout final
	{
		friend Factory;

	public:
		const InputLayoutDesc& GetDesc() const { return this->m_desc; };
		const PlatformInputLayout& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformInputLayout m_platformImpl;
		InputLayoutDesc m_desc;
	};

	struct ShaderDesc
	{

	};
	class Shader final
	{
		friend Factory;

	public:
		const ShaderDesc& GetDesc() const { return this->m_desc; };
		const PlatformShader& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformShader m_platformImpl;
		ShaderDesc m_desc;
	};

	struct CommandSignatureDesc
	{

	};
	class CommandSignature final
	{
		friend Factory;

	public:
		const CommandSignatureDesc& GetDesc() const { return this->m_desc; };
		const PlatformCommandSignature& GetPlatform() const { return this->m_platformImpl; };

	private:
		PlatformCommandSignature m_platformImpl;
		CommandSignatureDesc m_desc;
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

	class SwapChain final
	{
		friend Factory;

	public:
		const SwapchainDesc& GetDesc() const { return this->m_desc; };
		const Core::RefCountPtr<PlatformSwapChain>& GetPlatform() const { return this->m_platformImpl; };

	private:
		Core::RefCountPtr<PlatformSwapChain> m_platformImpl;
		SwapchainDesc m_desc;
	};

}