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
	struct RgNonCopyable
	{
		RgNonCopyable() = default;
		RgNonCopyable(RgNonCopyable const&) = delete; // non construction-copyable
		RgNonCopyable& operator=(RgNonCopyable const&) = delete; // non copyable
	};
	enum class RgReferenceType : uint32_t
	{
		Invalid,
		RenderTarget,
		PassResult,
		ExternalSRV

	};

	struct RgReference
	{
		enum : uint8_t
		{
			AllSubresources = 0x7f
		};

		union
		{
			struct 
			{
				RgReferenceType Type : 2;
				uint32_t Depth : 1;
				uint32_t EntireTexture : 1;		 // Used for PassResult types to specify AllSubresources (since SubResource is used for the output index)
				uint32_t MipSlice : 1;
				uint32_t SubResource : 7;
				uint32_t MipLevel : 4;
				uint32_t Index : 16;
			};
			uint32_t Data;
		};

		RgReference() {};

		constexpr RgReference(const RgReferenceType type, const uint32_t index, const uint32_t sub_resource, const uint32_t depth = 0, const uint32_t entire_texture = 0, const uint32_t mip_slice = 0, const uint32_t mip_level = 0)
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

		constexpr RgReference SubResourceRef(const uint32_t sub_resource) const { return RgReference(Type, Index, sub_resource, Depth); }
		constexpr RgReference MipSliceRef(const uint32_t mip_level) const { return RgReference(Type, Index, 0, Depth, 0, 1, mip_level); }

		static constexpr RgReference Null()
		{
			return RgReference(RgReferenceType::Invalid, 0, 0);
		}

		explicit operator bool() const { return Data != 0; }
	};

	static_assert(sizeof(RgReference) == sizeof(uint32_t));


	struct RgRenderTarget
	{
		int Index = -1;

		RgRenderTarget() {}
		explicit RgRenderTarget(const int index) : Index(index) {}

		constexpr operator RgReference() const { return RgReference(RgReferenceType::RenderTarget, Index, 0); }
		constexpr RgReference SubResource(const uint32_t sub_resource) const { return RgReference(RgReferenceType::RenderTarget, Index, sub_resource); }
		constexpr RgReference MipSlice(const uint32_t mip_level) const { return RgReference(RgReferenceType::RenderTarget, Index, 0, 0, 0, 1, mip_level); }
		constexpr RgReference AllSubResources() const { return SubResource(RgReference::AllSubresources); }
	};

	struct RgPassResult
	{
		int Index = -1;

		RgPassResult() {}
		explicit RgPassResult(const int index) : Index(index) {}

		constexpr RgReference Colour(const int index, const bool entire_texture = false) const { return RgReference(RgReferenceType::PassResult, Index, index, 0, entire_texture); }
		constexpr RgReference Depth(const bool entire_texture = false) const { return RgReference(RgReferenceType::PassResult, Index, 0, 1, entire_texture); }
	};
	/** Flags to annotate a pass with when calling AddPass. */
	enum class RgPassFlags : uint8_t
	{
		None = 0,
		Raster = 1 << 0,
		Compute = 1 << 1,
		AsyncCompute = 1 << 2,
		NeverCull = 1 << 3,
	};
	PHX_ENUM_CLASS_FLAGS(RgPassFlags);

	class RgRegistry
	{
	public:
	};

	class RgDependencyLevel;
	class RgPass : RgNonCopyable
	{
		friend RgDependencyLevel;
	public:
		RgPass(std::string_view name, Core::Span<RgReference> reads, Core::Span<RgReference> writes, RgPassFlags flags) noexcept
			: m_name(name)
			, m_flags(flags)
		{
			this->m_reads.resize(reads.Size());
			std::memcpy(this->m_reads.data(), reads.begin(), reads.Size() * sizeof(RgReference));

			this->m_writes.resize(writes.Size());
			std::memcpy(this->m_writes.data(), writes.begin(), writes.Size() * sizeof(RgReference));
		}

		bool HasAnyDependencies() const noexcept { return !this->m_reads.empty() || !this->m_writes.empty(); }

		Core::Span<RgReference> GetWrites() const noexcept { return this->m_writes; }
		Core::Span<RgReference> GetReads() const noexcept { return this->m_reads; }

	protected:
		virtual void Execute(RgRegistry const& registry, RHI::CommandListRef commandList) const {};

	private:
		RgPassFlags m_flags;
		std::string_view m_name;
		std::vector<RgReference> m_reads;
		std::vector<RgReference> m_writes;
	};

	template<typename TExecuteLambda>
	class RgLambdaPass final : public RgPass
	{
		// Verify that the amount of stuff captured by the pass lambda is reasonable.
		static constexpr size_t kMaximumLambdaCaptureSize = 1024;
		static_assert(sizeof(TExecuteLambda) <= kMaximumLambdaCaptureSize, "The amount of data of captured for the pass looks abnormally high.");

	public:
		RgLambdaPass(std::string_view name, Core::Span<RgReference> reads, Core::Span<RgReference> writes, RgPassFlags flags, TExecuteLambda&& lambda)
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

	enum class RgResourceType : uint8_t
	{
		Texture = 0,
		Buffer,
	};
	struct RgResourceDesc
	{
		std::string_view Name;
		RgResourceType Type;
		
		RHI::Format Format = RHI::Format::UNKNOWN;
		union
		{
			struct TextureDesc
			{
				RHI::TextureDimension dim = RHI::TextureDimension::Unknown;
				uint32_t Width = 1;
				uint32_t Height = 1;
				uint32_t DepthOrArraySize = 1;
				uint32_t MipLevels = 1;
				RHI::ClearValue ClearValue = {};
			} TextureDesc;

			struct bufferDesc
			{

			} BufferDesc;
		};

		static RgResourceDesc&& Texture2D(std::string_view name, RHI::Format format, uint32_t width, uint32_t height)
		{
			return {
				.Name = name,
				.Type = RgResourceType::Texture,
				.Format = format,
				.TextureDesc = {
					.dim = RHI::TextureDimension::Texture2D,
					.Width = width,
					.Height = height,
				}
			};
		}
	};

	class RgDependencyLevel
	{
	public:
		void AddPass(RgPass* pass);

		void Execute();

	private:
		std::vector<RgPass*> m_passes;

		std::vector<RgReference> m_reads;
		std::vector<RgReference> m_writes;
	};

	class RgBuilder
	{
	public:
		RgBuilder(Core::IAllocator* allocoator);
		RgBuilder(const RgBuilder&) = delete;

		void Execute();

		RgReference RegisterExternalTarget(RHI::Texture* texture)
		{
			RgReference reference(
				RgReferenceType::ExternalSRV,
				this->m_externalRenderTargetTextures.size(),
				0);

			this->m_externalRenderTargetTextures.push_back(texture);
			return reference;
		}

		inline RgRenderTarget CreateTarget(RgResourceDesc&& desc)
		{
			RgRenderTarget renderTarget(this->m_targetDescriptions.size());
			this->m_targetDescriptions.emplace_back(desc);
			return renderTarget;
		}

		template<typename TExecuteLambda>
		inline RgPassResult AddPass(std::string_view name, Core::Span<RgReference> reads, Core::Span<RgReference> writes, RgPassFlags flags, TExecuteLambda&& lambda)
		{
			using LambdaPassType = RgLambdaPass<TExecuteLambda>;
			RgPass* pass = this->m_graphAllocator->Construct<LambdaPassType>(name, reads, writes, flags, std::move(lambda));
			RgPassResult result(this->m_renderPasses.size());
			m_renderPasses.emplace_back(pass);

			return result;
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

		// Graph Resources
		std::vector<RHI::Texture*> m_externalRenderTargetTextures;
		std::vector<RgResourceDesc> m_targetDescriptions;
	};
}

