#pragma once

#include <assert.h>
// -- Switches based on backend ---
#include <PlatformTypes.h>

namespace PhxEngine::RHI
{
	enum class CommandContextType : uint8_t
	{
		Graphics = 0,
		Compute,
		Copy,

		Count
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
	public:
	private:
		PlatformGpuBuffer m_platformImpl;
	};

	struct TextureDesc
	{

	};

	class Texture
	{
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
	private:
	};

	struct ComputePipelineDesc
	{

	};
	class ComputePipeline
	{
	private:
	};

	struct MeshPipelineDesc
	{

	};
	class MeshPipeline
	{
	private:
	};

	struct InputLayoutDesc
	{

	};
	class InputLayout
	{
	private:
	};

	struct ShaderDesc
	{

	};
	class Shader
	{
	private:
	};

	struct CommandSignatureDesc
	{

	};
	class CommandSignature
	{

	};

	struct SwapchainDesc
	{
		uint32_t Width = 1u;
		uint32_t Height = 1u;
		void* WindowHandle = nullptr;
		size_t NumBackBuffers = 3;
		bool VSync = false;
		bool Fullscreen = false;
	};
	class SwapChain
	{
	private:
	};

}