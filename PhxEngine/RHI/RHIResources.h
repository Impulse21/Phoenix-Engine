#pragma once

#include <assert.h>
#include <string>
#include <variant>
#include <optional>

#include <Core/Span.h>
#include <Core/Handle.h>
#include <RHI/RHIEnums.h>

// -- Exposes Internal Platform Type alias Trick to avoid virtuals ---
#include <PlatformTypes.h>
namespace PhxEngine::RHI
{
	typedef uint32_t DescriptorIndex;

	static constexpr DescriptorIndex cInvalidDescriptorIndex = ~0u;

	static constexpr uint32_t cMaxRenderTargets = 8;
	static constexpr uint32_t cMaxViewports = 16;
	static constexpr uint32_t cMaxVertexAttributes = 16;
	static constexpr uint32_t cMaxBindingLayouts = 5;
	static constexpr uint32_t cMaxBindingsPerLayout = 128;
	static constexpr uint32_t cMaxVolatileConstantBuffersPerLayout = 6;
	static constexpr uint32_t cMaxVolatileConstantBuffers = 32;
	static constexpr uint32_t cMaxPushConstantSize = 128;      // D3D12: root signature is 256 bytes max., Vulkan: 128 bytes of push constants guaranteed

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

	struct SubresourceData
	{
		const void* pData = nullptr;
		uint32_t rowPitch = 0;
		uint32_t slicePitch = 0;
	};

	struct SubmitRecipt
	{
	};

	struct Viewport
	{
		float MinX, MaxX;
		float MinY, MaxY;
		float MinZ, MaxZ;

		Viewport() : MinX(0.f), MaxX(0.f), MinY(0.f), MaxY(0.f), MinZ(0.f), MaxZ(1.f) { }

		Viewport(float width, float height) : MinX(0.f), MaxX(width), MinY(0.f), MaxY(height), MinZ(0.f), MaxZ(1.f) { }

		Viewport(float _minX, float _maxX, float _minY, float _maxY, float _minZ, float _maxZ)
			: MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY), MinZ(_minZ), MaxZ(_maxZ)
		{ }

		bool operator ==(const Viewport& b) const
		{
			return MinX == b.MinX
				&& MinY == b.MinY
				&& MinZ == b.MinZ
				&& MaxX == b.MaxX
				&& MaxY == b.MaxY
				&& MaxZ == b.MaxZ;
		}
		bool operator !=(const Viewport& b) const { return !(*this == b); }

		float GetWidth() const { return MaxX - MinX; }
		float GetHeight() const { return MaxY - MinY; }
	};

	struct Rect
	{
		int MinX, MaxX;
		int MinY, MaxY;

		Rect() : MinX(0), MaxX(0), MinY(0), MaxY(0) { }
		Rect(int width, int height) : MinX(0), MaxX(width), MinY(0), MaxY(height) { }
		Rect(int _minX, int _maxX, int _minY, int _maxY) : MinX(_minX), MaxX(_maxX), MinY(_minY), MaxY(_maxY) { }
		explicit Rect(const Viewport& viewport)
			: MinX(int(floorf(viewport.MinX)))
			, MaxX(int(ceilf(viewport.MaxX)))
			, MinY(int(floorf(viewport.MinY)))
			, MaxY(int(ceilf(viewport.MaxY)))
		{
		}

		bool operator ==(const Rect& b) const {
			return MinX == b.MinX && MinY == b.MinY && MaxX == b.MaxX && MaxY == b.MaxY;
		}
		bool operator !=(const Rect& b) const { return !(*this == b); }

		int GetWidth() const { return MaxX - MinX; }
		int GetHeight() const { return MaxY - MinY; }
	};

	// -- Pipeline State objects ---
	struct BlendRenderState
	{
		struct RenderTarget
		{
			bool        BlendEnable = false;
			BlendFactor SrcBlend = BlendFactor::One;
			BlendFactor DestBlend = BlendFactor::Zero;
			EBlendOp    BlendOp = EBlendOp::Add;
			BlendFactor SrcBlendAlpha = BlendFactor::One;
			BlendFactor DestBlendAlpha = BlendFactor::Zero;
			EBlendOp    BlendOpAlpha = EBlendOp::Add;
			ColorMask   ColorWriteMask = ColorMask::All;
		};

		RenderTarget Targets[cMaxRenderTargets];
		bool alphaToCoverageEnable = false;
	};

	struct DepthStencilRenderState
	{
		struct StencilOpDesc
		{
			StencilOp FailOp = StencilOp::Keep;
			StencilOp DepthFailOp = StencilOp::Keep;
			StencilOp PassOp = StencilOp::Keep;
			ComparisonFunc StencilFunc = ComparisonFunc::Always;

			constexpr StencilOpDesc& setFailOp(StencilOp value) { FailOp = value; return *this; }
			constexpr StencilOpDesc& setDepthFailOp(StencilOp value) { DepthFailOp = value; return *this; }
			constexpr StencilOpDesc& setPassOp(StencilOp value) { PassOp = value; return *this; }
			constexpr StencilOpDesc& setStencilFunc(ComparisonFunc value) { StencilFunc = value; return *this; }
		};

		bool            DepthTestEnable = true;
		bool            DepthWriteEnable = true;
		ComparisonFunc  DepthFunc = ComparisonFunc::Less;
		bool            StencilEnable = false;
		uint8_t         StencilReadMask = 0xff;
		uint8_t         StencilWriteMask = 0xff;
		uint8_t         stencilRefValue = 0;
		StencilOpDesc   FrontFaceStencil;
		StencilOpDesc   BackFaceStencil;
	};

	struct RasterRenderState
	{
		RasterFillMode FillMode = RasterFillMode::Solid;
		RasterCullMode CullMode = RasterCullMode::Back;
		bool FrontCounterClockwise = false;
		bool DepthClipEnable = false;
		bool ScissorEnable = false;
		bool MultisampleEnable = false;
		bool AntialiasedLineEnable = false;
		int DepthBias = 0;
		float DepthBiasClamp = 0.f;
		float SlopeScaledDepthBias = 0.f;

		uint8_t ForcedSampleCount = 0;
		bool programmableSamplePositionsEnable = false;
		bool ConservativeRasterEnable = false;
		bool quadFillEnable = false;
		char samplePositionsX[16]{};
		char samplePositionsY[16]{};
	};
	// -- Pipeline State Objects End ---

	struct Shader;
	using ShaderHandle = Core::Handle<Shader>;
	struct ShaderDesc
	{
		ShaderStage Stage = ShaderStage::None;
		std::string DebugName = "";
	};


	struct InputLayout;
	using InputLayoutHandle = Core::Handle<InputLayout>;
	struct VertexAttributeDesc
	{
		static const uint32_t SAppendAlignedElement = 0xffffffff; // automatically figure out AlignedByteOffset depending on Format

		std::string SemanticName;
		uint32_t SemanticIndex = 0;
		RHI::Format Format = RHI::Format::UNKNOWN;
		uint32_t InputSlot = 0;
		uint32_t AlignedByteOffset = SAppendAlignedElement;
		bool IsInstanced = false;
	};

	struct Texture;
	using TextureHandle = Core::Handle<Texture>;
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

	struct Buffer;
	using BufferHandle = Core::Handle<Buffer>;
	struct BufferDesc
	{
		BufferMiscFlags MiscFlags = BufferMiscFlags::None;
		Usage Usage = Usage::Default;
		BindingFlags Binding = BindingFlags::None;
		ResourceStates InitialState = ResourceStates::Common;
		RHI::Format Format = RHI::Format::UNKNOWN;

		uint64_t Stride = 0;
		uint64_t NumElements = 0;
		size_t UavCounterOffset = 0;
		BufferHandle UavCounterBuffer = {};
		BufferHandle AliasedBuffer = {};

		std::string DebugName;
	};

	// This is just a workaround for now
	class IRootSignatureBuilder
	{
	public:
		~IRootSignatureBuilder() = default;
	};

	struct StaticSamplerParameter
	{
		uint32_t Slot;
		Color BorderColor = 1.f;
		float MaxAnisotropy = 1.f;
		float MipBias = 0.f;

		bool MinFilter = true;
		bool MagFilter = true;
		bool MipFilter = true;
		SamplerAddressMode AddressU = SamplerAddressMode::Clamp;
		SamplerAddressMode AddressV = SamplerAddressMode::Clamp;
		SamplerAddressMode AddressW = SamplerAddressMode::Clamp;
		ComparisonFunc ComparisonFunc = ComparisonFunc::LessOrEqual;
		SamplerReductionType ReductionType = SamplerReductionType::Standard;
	};
	struct ShaderParameter
	{
		uint32_t Slot;
		ResourceType Type : 8;
		bool IsVolatile : 8;
		uint16_t Size : 16;

	};

	struct BindlessShaderParameter
	{
		ResourceType Type : 8;
		uint16_t RegisterSpace : 16;
	};

	// Shader Binding Layout
	struct ShaderParameterLayout
	{
		ShaderStage Visibility = ShaderStage::None;
		uint32_t RegisterSpace = 0;
		std::vector<ShaderParameter> Parameters;
		std::vector<BindlessShaderParameter> BindlessParameters;
		std::vector<StaticSamplerParameter> StaticSamplers;

		ShaderParameterLayout& AddPushConstantParmaeter(uint32_t shaderRegister, size_t size)
		{
			ShaderParameter p = {};
			p.Size = size;
			p.Slot = shaderRegister;
			p.Type = ResourceType::PushConstants;

			this->AddParameter(std::move(p));

			return *this;
		}

		ShaderParameterLayout& AddCBVParameter(uint32_t shaderRegister, bool IsVolatile = false)
		{
			ShaderParameter p = {};
			p.Size = 0;
			p.Slot = shaderRegister;
			p.IsVolatile = IsVolatile;
			p.Type = ResourceType::ConstantBuffer;

			this->AddParameter(std::move(p));

			return *this;
		}

		ShaderParameterLayout& AddSRVParameter(uint32_t shaderRegister)
		{
			ShaderParameter p = {};
			p.Size = 0;
			p.Slot = shaderRegister;
			p.Type = ResourceType::SRVBuffer;

			this->AddParameter(std::move(p));

			return *this;
		}

		ShaderParameterLayout AddBindlessSRV(uint32_t shaderSpace)
		{
			BindlessShaderParameter p = {};
			p.RegisterSpace = shaderSpace;
			p.Type = ResourceType::BindlessSRV;

			this->AddBindlessParameter(std::move(p));

			return *this;
		}

		ShaderParameterLayout& AddStaticSampler(
			uint32_t shaderRegister,
			bool minFilter,
			bool magFilter,
			bool mipFilter,
			SamplerAddressMode         addressUVW,
			uint32_t				   maxAnisotropy = 16U,
			ComparisonFunc             comparisonFunc = ComparisonFunc::LessOrEqual,
			Color                      borderColor = { 1.0f , 1.0f, 1.0f, 0.0f })
		{
			StaticSamplerParameter& desc = this->StaticSamplers.emplace_back();
			desc.Slot = shaderRegister;
			desc.MinFilter = minFilter;
			desc.MagFilter = magFilter;
			desc.MagFilter = mipFilter;
			desc.AddressU = addressUVW;
			desc.AddressV = addressUVW;
			desc.AddressW = addressUVW;
			desc.MaxAnisotropy = static_cast<float>(maxAnisotropy);
			desc.ComparisonFunc = comparisonFunc;
			desc.BorderColor = borderColor;

			return *this;
		}

		void AddParameter(ShaderParameter&& parameter)
		{
			this->Parameters.emplace_back(parameter);
		}

		void AddBindlessParameter(BindlessShaderParameter&& bindlessParameter)
		{
			this->BindlessParameters.emplace_back(bindlessParameter);
		}
	};

	struct GfxPipeline;
	using GfxPipelineHandle = Core::Handle<GfxPipeline>;
	struct GfxPipelineDesc
	{
		PrimitiveType PrimType = PrimitiveType::TriangleList;
		InputLayoutHandle InputLayout;

		IRootSignatureBuilder* RootSignatureBuilder = nullptr;
		ShaderParameterLayout ShaderParameters;

		ShaderHandle VertexShader;
		ShaderHandle HullShader;
		ShaderHandle DomainShader;
		ShaderHandle GeometryShader;
		ShaderHandle PixelShader;

		BlendRenderState BlendRenderState = {};
		DepthStencilRenderState DepthStencilRenderState = {};
		RasterRenderState RasterRenderState = {};

		std::vector<RHI::Format> RtvFormats;
		std::optional<RHI::Format> DsvFormat;

		uint32_t SampleCount = 1;
		uint32_t SampleQuality = 0;
	};

	struct ComputePipeline;
	using ComputePipelineHandle = Core::Handle<ComputePipeline>;
	struct ComputePipelineDesc
	{
		ShaderHandle ComputeShader;
	};

	struct MeshPipeline;
	using MeshPipelineHandle = Core::Handle<MeshPipeline>;
	struct MeshPipelineDesc
	{
		PrimitiveType PrimType = PrimitiveType::TriangleList;

		ShaderHandle AmpShader;
		ShaderHandle MeshShader;
		ShaderHandle PixelShader;

		BlendRenderState BlendRenderState = {};
		DepthStencilRenderState DepthStencilRenderState = {};
		RasterRenderState RasterRenderState = {};

		std::vector<RHI::Format> RtvFormats;
		std::optional<RHI::Format> DsvFormat;

		uint32_t SampleCount = 1;
		uint32_t SampleQuality = 0;
	};

	struct SwapChain;
	using SwapChainHandle = Core::Handle<SwapChain>;
	struct SwapchainDesc
	{
		void* WindowHandle;
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

	struct RTAccelerationStructure;
	using RTAccelerationStructureHandle = Core::Handle<RTAccelerationStructure>;

	struct RTAccelerationStructureDesc
	{
		enum FLAGS
		{
			kEmpty = 0,
			kAllowUpdate = 1 << 0,
			kAllowCompaction = 1 << 1,
			kPreferFastTrace = 1 << 2,
			kPreferFastBuild = 1 << 3,
			kMinimizeMemory = 1 << 4,
		};
		uint32_t Flags = kEmpty;

		enum class Type
		{
			BottomLevel = 0,
			TopLevel
		} Type;

		struct BottomLevelDesc
		{
			struct Geometry
			{
				enum FLAGS
				{
					kEmpty = 0,
					kOpaque = 1 << 0,
					kNoduplicateAnyHitInvocation = 1 << 1,
					kUseTransform = 1 << 2,
				};
				uint32_t Flags = kEmpty;

				enum class Type
				{
					Triangles,
					ProceduralAABB,
				} Type = Type::Triangles;

				struct TrianglesDesc
				{
					BufferHandle VertexBuffer;
					BufferHandle IndexBuffer;
					uint32_t IndexCount = 0;
					uint64_t IndexOffset = 0;
					uint32_t VertexCount = 0;
					uint64_t VertexByteOffset = 0;
					uint32_t VertexStride = 0;
					RHI::Format IndexFormat = RHI::Format::R32_UINT;
					RHI::Format VertexFormat = RHI::Format::RGB32_FLOAT;
					BufferHandle Transform3x4Buffer;
					uint32_t Transform3x4BufferOffset = 0;
				} Triangles;

				struct ProceduralAABBsDesc
				{
					BufferHandle AABBBuffer;
					uint32_t Offset = 0;
					uint32_t Count = 0;
					uint32_t Stride = 0;
				} AABBs;
			};

			std::vector<Geometry> Geometries;
		} ButtomLevel;

		struct TopLevelDesc
		{
			struct Instance
			{
				enum FLAGS
				{
					kEmpty = 0,
					kTriangleCullDisable = 1 << 0,
					kTriangleFrontCounterClockwise = 1 << 1,
					kForceOpaque = 1 << 2,
					kForceNowOpaque = 1 << 3,
				};

				float Transform[3][4];
				uint32_t InstanceId : 24;
				uint32_t InstanceMask : 8;
				uint32_t InstanceContributionToHitGroupIndex : 24;
				uint32_t Flags : 8;
				RTAccelerationStructureHandle BottomLevel = {};
			};

			BufferHandle InstanceBuffer = {};
			uint32_t Offset = 0;
			uint32_t Count = 0;
		} TopLevel;
	};

	struct TimerQuery;
	using TimerQueryHandle = Core::Handle<TimerQuery>;

	struct GpuBarrier
	{
		struct BufferBarrier
		{
			RHI::BufferHandle Buffer;
			RHI::ResourceStates BeforeState;
			RHI::ResourceStates AfterState;
		};

		struct TextureBarrier
		{
			RHI::TextureHandle Texture;
			RHI::ResourceStates BeforeState;
			RHI::ResourceStates AfterState;
			int Mip;
			int Slice;
		};

		struct MemoryBarrier
		{
			std::variant<TextureHandle, BufferHandle> Resource;
		};

		std::variant<BufferBarrier, TextureBarrier, GpuBarrier::MemoryBarrier> Data;

		static GpuBarrier CreateMemory()
		{
			GpuBarrier barrier = {};
			barrier.Data = GpuBarrier::MemoryBarrier{};

			return barrier;
		}

		static GpuBarrier CreateMemory(TextureHandle texture)
		{
			GpuBarrier barrier = {};
			barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = texture };

			return barrier;
		}

		static GpuBarrier CreateMemory(BufferHandle buffer)
		{
			GpuBarrier barrier = {};
			barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = buffer };

			return barrier;
		}

		static GpuBarrier CreateTexture(
			TextureHandle texture,
			ResourceStates beforeState,
			ResourceStates afterState,
			int mip = -1,
			int slice = -1)
		{
			GpuBarrier::TextureBarrier t = {};
			t.Texture = texture;
			t.BeforeState = beforeState;
			t.AfterState = afterState;
			t.Mip = mip;
			t.Slice = slice;

			GpuBarrier barrier = {};
			barrier.Data = t;

			return barrier;
		}

		static GpuBarrier CreateBuffer(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState)
		{
			GpuBarrier::BufferBarrier b = {};
			b.Buffer = buffer;
			b.BeforeState = beforeState;
			b.AfterState = afterState;

			GpuBarrier barrier = {};
			barrier.Data = b;

			return barrier;
		}
	};

	struct IndirectArgumnetDesc
	{
		IndirectArgumentType Type;
		union
		{
			struct
			{
				uint32_t Slot;
			} 	VertexBuffer;
			struct
			{
				uint32_t RootParameterIndex;
				uint32_t DestOffsetIn32BitValues;
				uint32_t Num32BitValuesToSet;
			} 	Constant;
			struct
			{
				uint32_t RootParameterIndex;
			} 	ConstantBufferView;
			struct
			{
				uint32_t RootParameterIndex;
			} 	ShaderResourceView;
			struct
			{
				uint32_t RootParameterIndex;
			} 	UnorderedAccessView;
		};
	};
	struct CommandSignatureDesc
	{
		Core::Span<IndirectArgumnetDesc> ArgDesc;

		PipelineType PipelineType;
		union
		{
			RHI::GfxPipelineHandle GfxHandle;
			RHI::ComputePipelineHandle ComputeHandle;
			RHI::MeshPipelineHandle MeshHandle;
		};
	};

	struct CommandSignature;
	using CommandSignatureHandle = Core::Handle<CommandSignature>;

	struct RenderPass;
	using RenderPassHandle = Core::Handle<RenderPass>;
	struct RenderPassDesc
	{
		Core::Span<RHI::TextureHandle> RenderTargets;
		RHI::TextureHandle DepthTarget;
	};

	// -- Context Stuff
	struct PlatformContext {}; // TODO:

	struct GpuMemoryUsage
	{
		uint64_t Budget = 0ull;
		uint64_t Usage = 0ull;
	};

	struct NonCopyable
	{
		NonCopyable() = default;
		NonCopyable(const NonCopyable&) = delete;
		NonCopyable& operator=(const NonCopyable&) = delete;
	};

	struct NonMoveable
	{
		NonMoveable() = default;
		NonMoveable(NonMoveable&&) = delete;
		NonMoveable& operator=(NonMoveable&&) = delete;
	};

	class RenderPassContext
	{

	};

	struct DrawArgs
	{
		union
		{
			uint32_t VertexCount = 1;
			uint32_t IndexCount;
		};

		uint32_t InstanceCount = 1;
		union
		{
			uint32_t StartVertex = 0;
			uint32_t StartIndex;
		};
		uint32_t StartInstance = 0;

		uint32_t BaseVertex = 0;
	};


	class GfxContext;
	class ComputeContext;
	class CopyContext;
	class CommandList : NonCopyable, NonMoveable
	{
	public:
		void BeginMarker(std::string const& handle){ /*no-op*/ }
		void EndMarker(){ /*no-op*/ }

		// -- Ray Trace stuff       ---
		void RTBuildAccelerationStructure(RHI::RTAccelerationStructureHandle accelStructure){ /*no-op*/ }
		// GPUAllocation AllocateGpu(size_t bufferSize, size_t stride){ /*no-op*/ }

		void TransitionBarrier(TextureHandle texture, ResourceStates beforeState, ResourceStates afterState){ /*no-op*/ }
		void TransitionBarrier(BufferHandle buffer, ResourceStates beforeState, ResourceStates afterState){ /*no-op*/ }
		void TransitionBarriers(Core::Span<GpuBarrier> gpuBarriers){ /*no-op*/ }
		void ClearTextureFloat(TextureHandle texture, Color const& clearColour){ /*no-op*/ }
		void ClearDepthStencilTexture(TextureHandle depthStencil, bool clearDepth, float depth, bool clearStencil, uint8_t stencil){ /*no-op*/ }

		void Draw(DrawArgs const& args){ /*no-op*/ }
		void DrawIndexed(DrawArgs const& args){ /*no-op*/ }

		void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount){ /*no-op*/ }
		void ExecuteIndirect(RHI::CommandSignatureHandle commandSignature, RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount){ /*no-op*/ }

		void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount){ /*no-op*/ }
		void DrawIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount){ /*no-op*/ }

		void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, uint32_t maxCount){ /*no-op*/ }
		void DrawIndexedIndirect(RHI::BufferHandle args, size_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount){ /*no-op*/ }

		template<typename T>
		void WriteBuffer(BufferHandle buffer, std::vector<T> const& data, uint64_t destOffsetBytes)
		{
			this->WriteBuffer(buffer, data.data(), sizeof(T) * data.size(), destOffsetBytes);
		}

		template<typename T>
		void WriteBuffer(BufferHandle buffer, T const& data, uint64_t destOffsetBytes)
		{
			this->WriteBuffer(buffer, &data, sizeof(T), destOffsetBytes);
		}

		template<typename T>
		void WriteBuffer(BufferHandle buffer, Core::Span<T> data, uint64_t destOffsetBytes)
		{
			this->WriteBuffer(buffer, data.begin(), sizeof(T) * data.Size(), destOffsetBytes);
		}

		void WriteBuffer(BufferHandle buffer, const void* Data, size_t dataSize, uint64_t destOffsetBytes){ /*no-op*/ }

		void CopyBuffer(BufferHandle dst, uint64_t dstOffset, BufferHandle src, uint64_t srcOffset, size_t sizeInBytes){ /*no-op*/ }

		void WriteTexture(TextureHandle texture, uint32_t firstSubResource, size_t numSubResources, SubresourceData* pSubResourceData){ /*no-op*/ }
		void WriteTexture(TextureHandle texture, uint32_t arraySlice, uint32_t mipLevel, const void* Data, size_t rowPitch, size_t depthPitch){ /*no-op*/ }

		void SetGfxPipeline(GfxPipelineHandle gfxPipeline){ /*no-op*/ }
		void SetViewports(Viewport* viewports, size_t numViewports){ /*no-op*/ }
		void SetScissors(Rect* scissor, size_t numScissors){ /*no-op*/ }
		void SetRenderTargets(Core::Span<TextureHandle> renderTargets, TextureHandle depthStenc = {}){ /*no-op*/ }
		void SetRenderTargets(Core::Span<SwapChainHandle> renderTargets, TextureHandle depthStenc = {}){ /*no-op*/ }

		// -- Comptute Stuff ---
		void SetComputeState(ComputePipelineHandle state){ /*no-op*/ }
		void Dispatch(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ){ /*no-op*/ }
		void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount){ /*no-op*/ }
		void DispatchIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount){ /*no-op*/ }

		// -- Mesh Stuff ---
		void SetMeshPipeline(MeshPipelineHandle meshPipeline){ /*no-op*/ }
		void DispatchMesh(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ){ /*no-op*/ }
		void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, uint32_t maxCount){ /*no-op*/ }
		void DispatchMeshIndirect(RHI::BufferHandle args, uint32_t argsOffsetInBytes, RHI::BufferHandle count, size_t countOffsetInBytes, uint32_t maxCount){ /*no-op*/ }

		void BindPushConstant(uint32_t rootParameterIndex, uint32_t sizeInBytes, const void* constants){ /*no-op*/ }
		template<typename T>
		void BindPushConstant(uint32_t rootParameterIndex, const T& constants)
		{
			static_assert(sizeof(T) % sizeof(uint32_t) == 0, "Size of type must be a multiple of 4 bytes");
			this->BindPushConstant(rootParameterIndex, sizeof(T), &constants);
		}

		void BindConstantBuffer(size_t rootParameterIndex, BufferHandle constantBuffer){ /*no-op*/ }
		void BindDynamicConstantBuffer(size_t rootParameterIndex, size_t sizeInBytes, const void* bufferData){ /*no-op*/ }
		template<typename T>
		void BindDynamicConstantBuffer(size_t rootParameterIndex, T const& bufferData)
		{
			this->BindDynamicConstantBuffer(rootParameterIndex, sizeof(T), &bufferData);
		}

		void BindVertexBuffer(uint32_t slot, BufferHandle vertexBuffer){ /*no-op*/ }

		/**
		 * Set dynamic vertex buffer data to the rendering pipeline.
		 */
		void BindDynamicVertexBuffer(uint32_t slot, size_t numVertices, size_t vertexSize, const void* vertexBufferData){ /*no-op*/ }

		template<typename T>
		void BindDynamicVertexBuffer(uint32_t slot, const std::vector<T>& vertexBufferData)
		{
			this->BindDynamicVertexBuffer(slot, vertexBufferData.size(), sizeof(T), vertexBufferData.data());
		}

		void BindIndexBuffer(BufferHandle bufferHandle){ /*no-op*/ }

		/**
		 * Bind dynamic index buffer data to the rendering pipeline.
		 */
		void BindDynamicIndexBuffer(size_t numIndicies, RHI::Format indexFormat, const void* indexBufferData) { /*no-op*/ }

		template<typename T>
		void BindDynamicIndexBuffer(const std::vector<T>& indexBufferData)
		{
			static_assert(sizeof(T) == 2 || sizeof(T) == 4);

			RHI::Format indexFormat = (sizeof(T) == 2) ? RHI::Format::R16_UINT : RHI::Format::R32_UINT;
			this->BindDynamicIndexBuffer(indexBufferData.size(), indexFormat, indexBufferData.Data());
		}

		/**
		 * Set dynamic structured buffer contents.
		 */
		void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, size_t numElements, size_t elementSize, const void* bufferData){ /*no-op*/ }

		template<typename T>
		void BindDynamicStructuredBuffer(uint32_t rootParameterIndex, std::vector<T> const& bufferData)
		{
			this->BindDynamicStructuredBuffer(rootParameterIndex, bufferData.size(), sizeof(T), bufferData.Data());
		}

		void BindStructuredBuffer(size_t rootParameterIndex, BufferHandle buffer){ /*no-op*/ }

		void BindDynamicDescriptorTable(size_t rootParameterIndex, Core::Span<TextureHandle> textures){ /*no-op*/ }
		void BindDynamicUavDescriptorTable(
			size_t rootParameterIndex,
			Core::Span<BufferHandle> buffers)
		{
			this->BindDynamicUavDescriptorTable(rootParameterIndex, buffers, {});
		}
		void BindDynamicUavDescriptorTable(
			size_t rootParameterIndex,
			Core::Span<TextureHandle> textures)
		{
			this->BindDynamicUavDescriptorTable(rootParameterIndex, {}, textures);
		}

		void BindDynamicUavDescriptorTable(
			size_t rootParameterIndex,
			Core::Span<BufferHandle> buffers,
			Core::Span<TextureHandle> textures){ /*no-op*/ }

		void BindResourceTable(size_t rootParameterIndex){ /*no-op*/ }
		void BindSamplerTable(size_t rootParameterIndex){ /*no-op*/ }

		void BeginTimerQuery(TimerQueryHandle query){ /*no-op*/ }
		void EndTimerQuery(TimerQueryHandle query){ /*no-op*/ }

	private:
		PlatformCommandList m_platformCmdList;
	};
}