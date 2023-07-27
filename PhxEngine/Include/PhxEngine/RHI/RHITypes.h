#pragma once

#include <memory>
namespace PhxEngine::RHI
{
#pragma region Enums

    enum class Format : uint8_t
    {
        UNKNOWN = 0,
        R8_UINT,
        R8_SINT,
        R8_UNORM,
        R8_SNORM,
        RG8_UINT,
        RG8_SINT,
        RG8_UNORM,
        RG8_SNORM,
        R16_UINT,
        R16_SINT,
        R16_UNORM,
        R16_SNORM,
        R16_FLOAT,
        BGRA4_UNORM,
        B5G6R5_UNORM,
        B5G5R5A1_UNORM,
        RGBA8_UINT,
        RGBA8_SINT,
        RGBA8_UNORM,
        RGBA8_SNORM,
        BGRA8_UNORM,
        SRGBA8_UNORM,
        SBGRA8_UNORM,
        R10G10B10A2_UNORM,
        R11G11B10_FLOAT,
        RG16_UINT,
        RG16_SINT,
        RG16_UNORM,
        RG16_SNORM,
        RG16_FLOAT,
        R32_UINT,
        R32_SINT,
        R32_FLOAT,
        RGBA16_UINT,
        RGBA16_SINT,
        RGBA16_FLOAT,
        RGBA16_UNORM,
        RGBA16_SNORM,
        RG32_UINT,
        RG32_SINT,
        RG32_FLOAT,
        RGB32_UINT,
        RGB32_SINT,
        RGB32_FLOAT,
        RGBA32_UINT,
        RGBA32_SINT,
        RGBA32_FLOAT,

        D16,
        D24S8,
        X24G8_UINT,
        D32,
        D32S8,
        X32G8_UINT,

        BC1_UNORM,
        BC1_UNORM_SRGB,
        BC2_UNORM,
        BC2_UNORM_SRGB,
        BC3_UNORM,
        BC3_UNORM_SRGB,
        BC4_UNORM,
        BC4_SNORM,
        BC5_UNORM,
        BC5_SNORM,
        BC6H_UFLOAT,
        BC6H_SFLOAT,
        BC7_UNORM,
        BC7_UNORM_SRGB,

        COUNT,
    };
#pragma endregion

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

	struct RHIResource
	{
		std::shared_ptr<void> RHIResource;
		bool IsValid() const { return this->RHIResource != nullptr; }

		virtual ~RHIResource() = default;
	};

	struct GpuResource : public RHIResource
	{
		enum class Type
		{
			Unkown,
			Buffer,
			Texture,
			RTAccelerationStructure,
		} Type;

		constexpr bool IsTexture() const { return this->Type == Type::Texture; }
		constexpr bool IsBuffer() const { return this->Type == Type::Buffer; }
		constexpr bool IsAccelerationStructure() const { return this->Type == Type::RTAccelerationStructure; }
	};

	struct GpuTexture : public GpuResource
	{
		
	};

	struct GpuBuffer : public GpuResource
	{

	};

	struct SwapChainDesc
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
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

	struct SwapChain : public RHIResource
	{
        SwapChainDesc Desc;

        constexpr const SwapChainDesc& GetDesc() const { return this->Desc; }
	};
}