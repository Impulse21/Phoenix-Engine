#pragma once

#include <stack>
#include <vector>
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
	namespace RG
	{
		enum class ReferenceType : uint32_t
		{
			Invalid,
			RenderTarget,
			PassResult,
			ExternalSRV

		};

		struct Reference
		{
			enum : uint8_t
			{
				AllSubresources = 0x7f
			};

			union
			{
				struct
				{
					ReferenceType Type : 2;
					uint32_t Depth : 1;
					uint32_t EntireTexture : 1;		 // Used for PassResult types to specify AllSubresources (since SubResource is used for the output index)
					uint32_t MipSlice : 1;
					uint32_t SubResource : 7;
					uint32_t MipLevel : 4;
					uint32_t Index : 16;
				};
				uint32_t Data;
			};

			Reference() {};

			constexpr Reference(const ReferenceType type, const uint32_t index, const uint32_t sub_resource, const uint32_t depth = 0, const uint32_t entire_texture = 0, const uint32_t mip_slice = 0, const uint32_t mip_level = 0) 
				: Type(type)
				, Depth(depth)
				, EntireTexture(entire_texture)
				, MipSlice(mip_slice)
				, SubResource(sub_resource)
				, MipLevel(mip_level)
				, Index(index)
			{
				assert(sub_resource < 128);
				assert(index < 65536);
				assert(mip_slice == 1 || mip_level == 0);
				assert(mip_level < 16);
			}

			constexpr Reference SubResourceRef(const uint32_t sub_resource) const { return Reference(Type, Index, sub_resource, Depth); }
			constexpr Reference MipSliceRef(const uint32_t mip_level) const { return Reference(Type, Index, 0, Depth, 0, 1, mip_level); }

			static constexpr Reference Null()
			{
				return Reference(ReferenceType::Invalid, 0, 0);
			}

			explicit operator bool() const { return Data != 0; }
		};

		static_assert(sizeof(Reference) == sizeof(uint32_t));


		struct RenderTarget
		{
			int Index = -1;

			RenderTarget() {}
			explicit RenderTarget(const int index) : Index(index) {}

			constexpr operator Reference() const { return Reference(ReferenceType::RenderTarget, Index, 0); }
			constexpr Reference SubResource(const uint32_t sub_resource) const { return Reference(ReferenceType::RenderTarget, Index, sub_resource); }
			constexpr Reference MipSlice(const uint32_t mip_level) const { return Reference(ReferenceType::RenderTarget, Index, 0, 0, 0, 1, mip_level); }
			constexpr Reference AllSubResources() const { return SubResource(Reference::AllSubresources); }
		};

		struct PassResult
		{
			int Index = -1;

			PassResult() {}
			explicit PassResult(const int index) : Index(index) {}

			constexpr Reference Colour(const int index, const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, Index, index, 0, entire_texture); }
			constexpr Reference Depth(const bool entire_texture = false) const { return Reference(ReferenceType::PassResult, Index, 0, 1, entire_texture); }
		};

		class RenderGraphBuilder;
		class Pass : RgNonCopyable
		{
			friend RenderGraphBuilder;
		public:
			Pass& Reads(Core::Span<Reference> references);

			template<typename TExecuteLambda>
			Pass& Execute(TExecuteLambda&& func)
			{

			}

		private:
		};

		class RenderGraphBuilder
		{
		public:
			RenderGraphBuilder(Core::IAllocator* allocoator);
			RenderGraphBuilder(const RenderGraphBuilder&) = delete;

			void Execute();

			Pass& BeginPass();
			PassResult EndPass(Pass& pass);
		};
	}

	struct RgNonCopyable
	{
		RgNonCopyable() = default;
		RgNonCopyable(RgNonCopyable const&) = delete; // non construction-copyable
		RgNonCopyable& operator=(RgNonCopyable const&) = delete; // non copyable
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
		uint64_t		Id		: 32; // 32 bit uint32_t
	};

	// static_assert(sizeof(RgResourceHandle) == sizeof(uint64_t));


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

	class RgDependencyLevel;
	class RgPass : RgNonCopyable
	{
		friend RgDependencyLevel;
	public:
		RgPass(std::string_view name, Core::Span<RgResourceHandle> reads, Core::Span<RgResourceHandle> writes, RgPassFlags flags) noexcept
			: m_name(name)
			, m_flags(flags)
		{
			this->m_reads.resize(reads.Size());
			std::memcpy(this->m_reads.data(), reads.begin(), reads.Size() * sizeof(RgResourceHandle));

			this->m_writes.resize(writes.Size());
			std::memcpy(this->m_writes.data(), writes.begin(), writes.Size() * sizeof(RgResourceHandle));
		}

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

	class RgDependencyLevel
	{
	public:
		void AddPass(RgPass* pass);

		void Execute();

	private:
		std::vector<RgPass*> m_passes;

		std::vector<RgResourceHandle> m_reads;
		std::vector<RgResourceHandle> m_writes;
	};

	class RenderGraphBuilder
	{
	public:
		RenderGraphBuilder(Core::IAllocator* allocoator);
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
			using LambdaPassType = RgLambdaPass<TExecuteLambda>;
			RgPass* pass = this->m_graphAllocator->Construct<LambdaPassType>(name, reads, writes, flags, std::move(lambda));
			m_renderPasses.emplace_back(pass);

			return pass;
		}

	private:
		void Setup();

		void DepthFirstSearchRec(size_t n, std::vector<bool>& visited, std::stack<size_t>& stack) const;
		void DepthFirstSearchRec(size_t n, std::vector<bool>& visited, std::vector<bool>& onStack);

	private:
		Core::IAllocator* m_graphAllocator;
		// TODO: avoid These sort of allocations
		RgPass* m_prologuePass;
		RgPass* m_epiloguePass;
		std::vector<RgPass*> m_renderPasses;

		std::vector<std::vector<size_t>> m_adjacencyLists;
		std::vector<RgPass*> m_topologicalSortedPasses;
		std::vector<RgDependencyLevel> m_compiledDependencyLevels;
	};
}

