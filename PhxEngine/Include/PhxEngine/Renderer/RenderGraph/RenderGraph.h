#pragma once

#include <string>
#include <PhxEngine/Core/Span.h>

#include <PhxEngine/RHI/PhxRHIResources.h>
#include <PhxEngine/Core/Memory.h>

namespace PhxEngine::RHI
{
	class Texture;
	class Buffer;
}

namespace PhxEngine::Renderer
{
	struct RgNonCopyable
	{
		RgNonCopyable(RgNonCopyable const&); // non construction-copyable
		RgNonCopyable& operator=(RgNonCopyable const&); // non copyable
	};
	// Ether do pointers or handles
	class RGTexture
	{
	public:
		RGTexture() = default;
	};

	class RenderPass
	{
	public:
		RenderPass(std::string_view view);

	private:
		std::string_view m_passName;
	};

	class RgRegistry : RgNonCopyable
	{
	public:
		RgRegistry() = default;
	};

	enum class RgResourceType : uint16_t
	{
		Unknown,
		Buffer,
		Texture,
	};

	enum RgResourceFlags : uint8_t
	{
		RG_RESOURCE_FLAG_NONE,
		RG_RESOURCE_FLAG_IMPORTED
	};

	// A virtual resource handle, the underlying realization of the resource type is done in RenderGraphRegistry
	struct RgResourceHandle
	{
		auto operator<=>(const RgResourceHandle&) const noexcept = default;

		[[nodiscard]] bool IsValid() const noexcept { return Type != RgResourceType::Unknown && Id != UINT_MAX; }
		[[nodiscard]] bool IsImported() const noexcept { return Flags & RG_RESOURCE_FLAG_IMPORTED; }

		void Invalidate()
		{
			Type = RgResourceType::Unknown;
			Flags = RG_RESOURCE_FLAG_NONE;
			Version = 0;
			Id = UINT_MAX;
		}

		RgResourceType	Type	: 15; // 14 bit to represent type, might remove some bits from this and give it to version
		RgResourceFlags Flags	: 1;
		uint64_t		Version : 16; // 16 bits to represent version should be more than enough, we can always just increase bit used if is not enough
		uint64_t		Id		: 32; // 32 bit unsigned int
	};

	static_assert(sizeof(RgResourceHandle) == sizeof(uint64_t));


	/** Flags to annotate a pass with when calling AddPass. */
	enum class RgPassFlags : uint8_t
	{
		None			= 0,
		Raster			= 1 << 0,
		Compute			= 1 << 1,
		AsyncCompute	= 1 << 2, 
		NeverCull		= 1 << 3,
	};
	PHX_ENUM_CLASS_FLAGS(RgPassFlags);

	class RgPass : RgNonCopyable
	{
	public:
		RgPass(std::string_view name, Core::Span<RgResourceHandle> reads, Core::Span<RgResourceHandle> writes, RgPassFlags flags) noexcept;

		bool HasAnyDependencies() const noexcept { return !this->m_reads.empty() || !this->m_writes.empty(); }

		Core::Span<RgResourceHandle> GetWrites() const noexcept { return this->m_writes; }
		Core::Span<RgResourceHandle> GetReads() const noexcept { return this->m_writes; }

	protected:
		virtual void Execute(RgRegistry const& registry, RHI::CommandListRef commandList) const {};

	private:
		RgPassFlags m_flags;
		std::string_view m_name;
		std::vector<RgResourceHandle> m_reads;
		std::vector<RgResourceHandle> m_writes;


	};
	template<typename TExecuteLambda>
	class RgLambdaPass final : public RgPass
	{
		// Verify that the amount of stuff captured by the pass lambda is reasonable.
		static constexpr size_t kMaximumLambdaCaptureSize = 1024;
		static_assert(sizeof(TExecuteLambda) <= kMaximumLambdaCaptureSize, "The amount of data of captured for the pass looks abnormally high.");

	public:
		RgLambdaPass(std::string_view name, Core::Span<RgResourceHandle> reads, Core::Span<RgResourceHandle> writes, RgPassFlags flags, TExecuteLambda&& lambda)
			: RgPass(name, reads, writes, flags)
			, m_executeLambda(std::move(lambda))
		{}

	protected:
		void Execute(RgRegistry const& registry, RHI::CommandListRef commandList) const override
		{
			this->m_executeLambda(registry, commandList);
		}

	private:
		TExecuteLambda m_executeLambda;
	};

	class RgSentinelPass final : public RgPass
	{
	public:
		RgSentinelPass(std::string_view name, RgPassFlags flags = RgPassFlags::None)
			: RgPass(name, {}, {}, RgPassFlags::NeverCull | flags)
		{
		}
	};

	class RenderGraphBuilder
	{
	public:
		RenderGraphBuilder();
		RenderGraphBuilder(const RenderGraphBuilder&) = delete;

		void Execute();

		RgResourceHandle RegisterExternalTexture(RHI::Texture* texture)
		{
			RgResourceHandle handle = {
				.Type = RgResourceType::Texture,
				.Flags = RG_RESOURCE_FLAG_IMPORTED,
				.Version = 0,
				.Id = ~0u,
			};

			return handle;
		}

		inline RgResourceHandle CreateTexture()
		{
			RgResourceHandle handle = {
				.Type = RgResourceType::Texture,
				.Flags = RG_RESOURCE_FLAG_NONE,
				.Version = 0,
				.Id = ~0u,
			};

			return handle;
		}

		inline RgResourceHandle CreateBuffer()
		{
			RgResourceHandle handle = {
				.Type = RgResourceType::Buffer,
				.Flags = RG_RESOURCE_FLAG_NONE,
				.Version = 0,
				.Id = ~0u,
			};

			return handle;
		}

		template<typename TExecuteLambda>
		inline RgPass* AddPass(std::string_view name, Core::Span<RgResourceHandle> reads, Core::Span<RgResourceHandle> writes, RgPassFlags flags, TExecuteLambda&& lambda)
		{
			// TODO: Allocate on
			RgPass* pass = m_graphAllocator->Allocate<RgLambdaPass>(name, reads, writes, flags, std::move(lambda));
			m_renderPasses.emplace_back(pass);

			return pass;
		}

	private:
		void Setup();
		void ExecuteInternal();

	private:
		Core::IAllocator* m_graphAllocator;
		// TODO: avoid These sort of allocations
		RgPass* m_prologuePass;
		RgPass* m_epiloguePass;
		std::vector<RgPass*> m_renderPasses;

		std::vector<std::vector<size_t>> m_adjacencyLists;

	};
}

