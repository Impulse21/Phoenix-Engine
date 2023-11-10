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

	template<typename TPlatform, typename TDesc>
	class TypedPlatformResource
	{
	public:
		virtual ~TypedPlatformResource() = default;

		const TDesc& Desc() const { return this->m_desc; }
		TPlatform& PlatformResource() { return this->m_platform; }
		const TPlatform& PlatformResource() const { return this->m_platform; }

		explicit operator bool() const
		{
			return !!this->m_platform;
		}

	protected:
		TPlatform m_platform;
		TDesc m_desc;
	};

	struct ShaderDesc
	{
		ShaderStage Stage = ShaderStage::None;
		std::string DebugName = "";
	};

	class Shader final : public TypedPlatformResource<RHI::PlatformShader, ShaderDesc>
	{
		friend Factory;
	public:
		Shader() = default;
	};


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

	class InputLayout final : public TypedPlatformResource<RHI::PlatformInputLayout, VertexAttributeDesc>
	{
		friend Factory;
	public:
		InputLayout() = default;
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

	class Texture final : public TypedPlatformResource<RHI::PlatformTexture, TextureDesc>
	{
		friend Factory;
	public:
		Texture() = default;
	};

	class Buffer;
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
		Buffer* UavCounterBuffer = {};
		Buffer* AliasedBuffer = {};

		std::string DebugName;
	};

	class Buffer final : public TypedPlatformResource<RHI::PlatformBuffer, BufferDesc>
	{
		friend Factory;
	public:
		Buffer() = default;
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

	struct GfxPipelineDesc
	{
		PrimitiveType PrimType = PrimitiveType::TriangleList;
		ShaderParameterLayout ShaderParameters;

		Shader* VertexShader;
		Shader* HullShader;
		Shader* DomainShader;
		Shader* GeometryShader;
		Shader* PixelShader;

		BlendRenderState BlendRenderState = {};
		DepthStencilRenderState DepthStencilRenderState = {};
		RasterRenderState RasterRenderState = {};

		std::vector<RHI::Format> RtvFormats;
		std::optional<RHI::Format> DsvFormat;

		uint32_t SampleCount = 1;
		uint32_t SampleQuality = 0;
	};

	class GfxPipeline final : public TypedPlatformResource<RHI::PlatformGfxPipeline, GfxPipelineDesc>
	{
		friend Factory;
	public:
		GfxPipeline() = default;
	};

	struct ComputePipelineDesc
	{
		Shader* ComputeShader;
	};

	class ComputePipeline final : public TypedPlatformResource<RHI::PlatformComputePipeline, ComputePipelineDesc>
	{
		friend Factory;
	public:
		ComputePipeline() = default;
	};

	struct MeshPipelineDesc
	{
		PrimitiveType PrimType = PrimitiveType::TriangleList;

		Shader* AmpShader;
		Shader* MeshShader;
		Shader* PixelShader;

		BlendRenderState BlendRenderState = {};
		DepthStencilRenderState DepthStencilRenderState = {};
		RasterRenderState RasterRenderState = {};

		std::vector<RHI::Format> RtvFormats;
		std::optional<RHI::Format> DsvFormat;

		uint32_t SampleCount = 1;
		uint32_t SampleQuality = 0;
	};

	class MeshPipeline final : public TypedPlatformResource<RHI::PlatformMeshPipeline, MeshPipelineDesc>
	{
		friend Factory;
	public:
		MeshPipeline() = default;
	};

	struct SwapchainDesc
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

	class SwapChain final : public TypedPlatformResource<RHI::PlatformSwapChain, SwapchainDesc>
	{
		friend Factory;
	public:
		SwapChain() = default;

	public:
		const Texture& GetCurrentBackBuffer()
		{
			this->m_platform.GetCurrentBackBuffer();
		}
	};

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
					Buffer* VertexBuffer;
					Buffer* IndexBuffer;
					uint32_t IndexCount = 0;
					uint64_t IndexOffset = 0;
					uint32_t VertexCount = 0;
					uint64_t VertexByteOffset = 0;
					uint32_t VertexStride = 0;
					RHI::Format IndexFormat = RHI::Format::R32_UINT;
					RHI::Format VertexFormat = RHI::Format::RGB32_FLOAT;
					Buffer* Transform3x4Buffer;
					uint32_t Transform3x4BufferOffset = 0;
				} Triangles;

				struct ProceduralAABBsDesc
				{
					Buffer* AABBBuffer;
					uint32_t Offset = 0;
					uint32_t Count = 0;
					uint32_t Stride = 0;
				} AABBs;
			};

			std::vector<Geometry> Geometries;
		} ButtomLevel;

		class RTAccelerationStructure;
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
				RTAccelerationStructure* BottomLevel = {};
			};

			Buffer* InstanceBuffer = {};
			uint32_t Offset = 0;
			uint32_t Count = 0;
		} TopLevel;
	};
	class RTAccelerationStructure final : public TypedPlatformResource<RHI::PlatformRTAccelerationStructure, RTAccelerationStructureDesc>
	{
		friend Factory;
	public:
		RTAccelerationStructure() = default;
	};

	struct TimerQueryDesc {};
	class TimerQuery final : public TypedPlatformResource<RHI::PlatformTimerQuery, TimerQueryDesc>
	{
		friend Factory;
	public:
		TimerQuery() = default;
	};

	struct GpuBarrier
	{
		struct BufferBarrier
		{
			RHI::Buffer* Buffer;
			RHI::ResourceStates BeforeState;
			RHI::ResourceStates AfterState;
		};

		struct TextureBarrier
		{
			RHI::Texture* Texture;
			RHI::ResourceStates BeforeState;
			RHI::ResourceStates AfterState;
			int Mip;
			int Slice;
		};

		struct MemoryBarrier
		{
			std::variant<Texture*, Buffer*> Resource;
		};

		std::variant<BufferBarrier, TextureBarrier, GpuBarrier::MemoryBarrier> Data;

		static GpuBarrier CreateMemory()
		{
			GpuBarrier barrier = {};
			barrier.Data = GpuBarrier::MemoryBarrier{};

			return barrier;
		}

		static GpuBarrier CreateMemory(Texture* texture)
		{
			GpuBarrier barrier = {};
			barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = texture };

			return barrier;
		}

		static GpuBarrier CreateMemory(Buffer* buffer)
		{
			GpuBarrier barrier = {};
			barrier.Data = GpuBarrier::MemoryBarrier{ .Resource = buffer };

			return barrier;
		}

		static GpuBarrier CreateTexture(
			Texture* texture,
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

		static GpuBarrier CreateBuffer(Buffer* buffer, ResourceStates beforeState, ResourceStates afterState)
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
			RHI::GfxPipeline* GfxHandle;
			RHI::ComputePipeline* ComputeHandle;
			RHI::MeshPipeline* MeshHandle;
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

	class CommandList
	{
	public:
	};
}