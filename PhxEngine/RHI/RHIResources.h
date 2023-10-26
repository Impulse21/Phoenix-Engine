#pragma once

#include <assert.h>
#include <string>
#include <variant>
#include <optional>

#include <Core/Span.h>
#include <Core/Handle.h>
#include <RHI/RHIEnums.h>

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
		virtual ~IRootSignatureBuilder() = default;
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


}