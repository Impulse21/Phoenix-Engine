#pragma once
#include <assert.h>

#include <Core/Window.h>
#include <Core/Memory.h>

#include <PlatformTypes.h>
namespace PhxEngine::RHI
{
	struct RHIParams
	{
		uint32_t NumBuffers;
		IWindow* Window;
		size_t ResourcePoolSize = PhxKB(1);
	};

	void Initialize(RHIParams const& params);
	void Finialize();

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

	class GpuBuffer
	{
	public:
	private:
		Platform m_platformBuffer;
	};

	class Texture
	{
	public:
	private:
	};

	class ResourceFactory
	{
	public:
		bool CreateGpuBuffer(GpuBuffer& buffer);
		// Create Resoruces
	};
}