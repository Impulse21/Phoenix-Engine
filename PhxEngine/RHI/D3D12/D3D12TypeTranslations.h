#pragma once

#include "RHI/PhxRHIEnums.h"
#include "PhxRHI_D3D12.h"
#include "assert.h"
#include "d3dx12.h"

namespace phx::rhi::d3d12
{
    struct DxgiFormatMapping
    {
        phx::rhi::Format AbstractFormat;
        DXGI_FORMAT ResourceFormat;
        DXGI_FORMAT SrvFormat;
        DXGI_FORMAT RtvFormat;
    };

    // Format mapping table. The rows must be in the exactly same order as Format enum members are defined.
    static const DxgiFormatMapping c_FormatMappings[] = {
        // Abstract format                Resource Format Type                SRV Format Type                       RTV Format Type
      { phx::rhi::Format::UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN,                  DXGI_FORMAT_UNKNOWN                },

      { phx::rhi::Format::R8_UINT,              DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UINT,                  DXGI_FORMAT_R8_UINT                },
      { phx::rhi::Format::R8_SINT,              DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_SINT,                  DXGI_FORMAT_R8_SINT                },
      { phx::rhi::Format::R8_UNORM,             DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UNORM,                 DXGI_FORMAT_R8_UNORM               },
      { phx::rhi::Format::R8_SNORM,             DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_SNORM,                 DXGI_FORMAT_R8_SNORM               },
      { phx::rhi::Format::RG8_UINT,             DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UINT,                DXGI_FORMAT_R8G8_UINT              },
      { phx::rhi::Format::RG8_SINT,             DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_SINT,                DXGI_FORMAT_R8G8_SINT              },
      { phx::rhi::Format::RG8_UNORM,            DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UNORM,               DXGI_FORMAT_R8G8_UNORM             },
      { phx::rhi::Format::RG8_SNORM,            DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_SNORM,               DXGI_FORMAT_R8G8_SNORM             },
      { phx::rhi::Format::R16_UINT,             DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UINT,                 DXGI_FORMAT_R16_UINT               },
      { phx::rhi::Format::R16_SINT,             DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_SINT,                 DXGI_FORMAT_R16_SINT               },
      { phx::rhi::Format::R16_UNORM,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_R16_UNORM              },
      { phx::rhi::Format::R16_SNORM,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_SNORM,                DXGI_FORMAT_R16_SNORM              },
      { phx::rhi::Format::R16_FLOAT,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_FLOAT,                DXGI_FORMAT_R16_FLOAT              },
      { phx::rhi::Format::BGRA4_UNORM,          DXGI_FORMAT_B4G4R4A4_UNORM,         DXGI_FORMAT_B4G4R4A4_UNORM,           DXGI_FORMAT_B4G4R4A4_UNORM         },
      { phx::rhi::Format::B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM,           DXGI_FORMAT_B5G6R5_UNORM,             DXGI_FORMAT_B5G6R5_UNORM           },
      { phx::rhi::Format::B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM,         DXGI_FORMAT_B5G5R5A1_UNORM,           DXGI_FORMAT_B5G5R5A1_UNORM         },
      { phx::rhi::Format::RGBA8_UINT,           DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UINT,            DXGI_FORMAT_R8G8B8A8_UINT          },
      { phx::rhi::Format::RGBA8_SINT,           DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_SINT,            DXGI_FORMAT_R8G8B8A8_SINT          },
      { phx::rhi::Format::RGBA8_UNORM,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM         },
      { phx::rhi::Format::RGBA8_SNORM,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_SNORM,           DXGI_FORMAT_R8G8B8A8_SNORM         },
      { phx::rhi::Format::BGRA8_UNORM,          DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM         },
      { phx::rhi::Format::SRGBA8_UNORM,         DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB    },
      { phx::rhi::Format::SBGRA8_UNORM,         DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB    },
      { phx::rhi::Format::R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_TYPELESS,   DXGI_FORMAT_R10G10B10A2_UNORM,        DXGI_FORMAT_R10G10B10A2_UNORM      },
      { phx::rhi::Format::R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,        DXGI_FORMAT_R11G11B10_FLOAT,          DXGI_FORMAT_R11G11B10_FLOAT        },
      { phx::rhi::Format::RG16_UINT,            DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_R16G16_UINT            },
      { phx::rhi::Format::RG16_SINT,            DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_R16G16_SINT            },
      { phx::rhi::Format::RG16_UNORM,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UNORM,             DXGI_FORMAT_R16G16_UNORM           },
      { phx::rhi::Format::RG16_SNORM,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_SNORM,             DXGI_FORMAT_R16G16_SNORM           },
      { phx::rhi::Format::RG16_FLOAT,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_FLOAT,             DXGI_FORMAT_R16G16_FLOAT           },
      { phx::rhi::Format::R32_UINT,             DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,                 DXGI_FORMAT_R32_UINT               },
      { phx::rhi::Format::R32_SINT,             DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_SINT,                 DXGI_FORMAT_R32_SINT               },
      { phx::rhi::Format::R32_FLOAT,            DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_R32_FLOAT              },
      { phx::rhi::Format::RGBA16_UINT,          DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UINT,        DXGI_FORMAT_R16G16B16A16_UINT      },
      { phx::rhi::Format::RGBA16_SINT,          DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_SINT,        DXGI_FORMAT_R16G16B16A16_SINT      },
      { phx::rhi::Format::RGBA16_FLOAT,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT     },
      { phx::rhi::Format::RGBA16_UNORM,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UNORM,       DXGI_FORMAT_R16G16B16A16_UNORM     },
      { phx::rhi::Format::RGBA16_SNORM,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_SNORM,       DXGI_FORMAT_R16G16B16A16_SNORM     },
      { phx::rhi::Format::RG32_UINT,            DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_R32G32_UINT            },
      { phx::rhi::Format::RG32_SINT,            DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_R32G32_SINT            },
      { phx::rhi::Format::RG32_FLOAT,           DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_FLOAT,             DXGI_FORMAT_R32G32_FLOAT           },
      { phx::rhi::Format::RGB32_UINT,           DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_UINT,           DXGI_FORMAT_R32G32B32_UINT         },
      { phx::rhi::Format::RGB32_SINT,           DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_SINT,           DXGI_FORMAT_R32G32B32_SINT         },
      { phx::rhi::Format::RGB32_FLOAT,          DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT        },
      { phx::rhi::Format::RGBA32_UINT,          DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_UINT,        DXGI_FORMAT_R32G32B32A32_UINT      },
      { phx::rhi::Format::RGBA32_SINT,          DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_SINT,        DXGI_FORMAT_R32G32B32A32_SINT      },
      { phx::rhi::Format::RGBA32_FLOAT,         DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT     },

      { phx::rhi::Format::D16,                  DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_D16_UNORM              },
      { phx::rhi::Format::D24S8,                DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_D24_UNORM_S8_UINT      },
      { phx::rhi::Format::X24G8_UINT,           DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_X24_TYPELESS_G8_UINT,     DXGI_FORMAT_D24_UNORM_S8_UINT      },
      { phx::rhi::Format::D32,                  DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_D32_FLOAT              },
      /*
      { phx::rhi::Format::D32,                  DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,                 DXGI_FORMAT_D32_FLOAT              },
      */
      { phx::rhi::Format::D32S8,                DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT   },
      { phx::rhi::Format::X32G8_UINT,           DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  DXGI_FORMAT_D32_FLOAT_S8X24_UINT   },

      { phx::rhi::Format::BC1_UNORM,            DXGI_FORMAT_BC1_TYPELESS,           DXGI_FORMAT_BC1_UNORM,                DXGI_FORMAT_BC1_UNORM              },
      { phx::rhi::Format::BC1_UNORM_SRGB,       DXGI_FORMAT_BC1_TYPELESS,           DXGI_FORMAT_BC1_UNORM_SRGB,           DXGI_FORMAT_BC1_UNORM_SRGB         },
      { phx::rhi::Format::BC2_UNORM,            DXGI_FORMAT_BC2_TYPELESS,           DXGI_FORMAT_BC2_UNORM,                DXGI_FORMAT_BC2_UNORM              },
      { phx::rhi::Format::BC2_UNORM_SRGB,       DXGI_FORMAT_BC2_TYPELESS,           DXGI_FORMAT_BC2_UNORM_SRGB,           DXGI_FORMAT_BC2_UNORM_SRGB         },
      { phx::rhi::Format::BC3_UNORM,            DXGI_FORMAT_BC3_TYPELESS,           DXGI_FORMAT_BC3_UNORM,                DXGI_FORMAT_BC3_UNORM              },
      { phx::rhi::Format::BC3_UNORM_SRGB,       DXGI_FORMAT_BC3_TYPELESS,           DXGI_FORMAT_BC3_UNORM_SRGB,           DXGI_FORMAT_BC3_UNORM_SRGB         },
      { phx::rhi::Format::BC4_UNORM,            DXGI_FORMAT_BC4_TYPELESS,           DXGI_FORMAT_BC4_UNORM,                DXGI_FORMAT_BC4_UNORM              },
      { phx::rhi::Format::BC4_SNORM,            DXGI_FORMAT_BC4_TYPELESS,           DXGI_FORMAT_BC4_SNORM,                DXGI_FORMAT_BC4_SNORM              },
      { phx::rhi::Format::BC5_UNORM,            DXGI_FORMAT_BC5_TYPELESS,           DXGI_FORMAT_BC5_UNORM,                DXGI_FORMAT_BC5_UNORM              },
      { phx::rhi::Format::BC5_SNORM,            DXGI_FORMAT_BC5_TYPELESS,           DXGI_FORMAT_BC5_SNORM,                DXGI_FORMAT_BC5_SNORM              },
      { phx::rhi::Format::BC6H_UFLOAT,          DXGI_FORMAT_BC6H_TYPELESS,          DXGI_FORMAT_BC6H_UF16,                DXGI_FORMAT_BC6H_UF16              },
      { phx::rhi::Format::BC6H_SFLOAT,          DXGI_FORMAT_BC6H_TYPELESS,          DXGI_FORMAT_BC6H_SF16,                DXGI_FORMAT_BC6H_SF16              },
      { phx::rhi::Format::BC7_UNORM,            DXGI_FORMAT_BC7_TYPELESS,           DXGI_FORMAT_BC7_UNORM,                DXGI_FORMAT_BC7_UNORM              },
      { phx::rhi::Format::BC7_UNORM_SRGB,       DXGI_FORMAT_BC7_TYPELESS,           DXGI_FORMAT_BC7_UNORM_SRGB,           DXGI_FORMAT_BC7_UNORM_SRGB         },
    };

    const DxgiFormatMapping& GetDxgiFormatMapping(phx::rhi::Format abstractFormat)
    {
        static_assert(sizeof(c_FormatMappings) / sizeof(DxgiFormatMapping) == size_t(phx::rhi::Format::COUNT),
            "The format mapping table doesn't have the right number of elements");

        const DxgiFormatMapping& mapping = c_FormatMappings[uint32_t(abstractFormat)];
        assert(mapping.AbstractFormat == abstractFormat);
        return mapping;
    }


    inline D3D12_RESOURCE_STATES ConvertResourceStates(phx::rhi::ResourceStates stateBits)
    {
        if (stateBits == phx::rhi::ResourceStates::Common)
            return D3D12_RESOURCE_STATE_COMMON;

        D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON; // also 0

        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::ConstantBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::VertexBuffer)) result |= D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::IndexGpuBuffer)) result |= D3D12_RESOURCE_STATE_INDEX_BUFFER;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::IndirectArgument)) result |= D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::ShaderResource)) result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::UnorderedAccess)) result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::RenderTarget)) result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::DepthWrite)) result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::DepthRead)) result |= D3D12_RESOURCE_STATE_DEPTH_READ;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::StreamOut)) result |= D3D12_RESOURCE_STATE_STREAM_OUT;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::CopyDest)) result |= D3D12_RESOURCE_STATE_COPY_DEST;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::CopySource)) result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::ResolveDest)) result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::ResolveSource)) result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::Present)) result |= D3D12_RESOURCE_STATE_PRESENT;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::AccelStructRead)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::AccelStructWrite)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::AccelStructBuildInput)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::AccelStructBuildBlas)) result |= D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::ShadingRateSurface)) result |= D3D12_RESOURCE_STATE_SHADING_RATE_SOURCE;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::GenericRead)) result |= D3D12_RESOURCE_STATE_GENERIC_READ;
        if (phx::core::EnumHasAnyFlags(stateBits, phx::rhi::ResourceStates::ShaderResourceNonPixel)) result |= D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

        return result;
    }
}
