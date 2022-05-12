#include "phxpch.h"
#include "Common.h"

using namespace PhxEngine::RHI;
using namespace PhxEngine::RHI::Dx12;

// Format mapping table. The rows must be in the exactly same order as Format enum members are defined.
static const DxgiFormatMapping c_FormatMappings[] = {
    { FormatType::UNKNOWN,              DXGI_FORMAT_UNKNOWN,                DXGI_FORMAT_UNKNOWN,                  DXGI_FORMAT_UNKNOWN                },

    { FormatType::R8_UINT,              DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UINT,                  DXGI_FORMAT_R8_UINT                },
    { FormatType::R8_SINT,              DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_SINT,                  DXGI_FORMAT_R8_SINT                },
    { FormatType::R8_UNORM,             DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_UNORM,                 DXGI_FORMAT_R8_UNORM               },
    { FormatType::R8_SNORM,             DXGI_FORMAT_R8_TYPELESS,            DXGI_FORMAT_R8_SNORM,                 DXGI_FORMAT_R8_SNORM               },
    { FormatType::RG8_UINT,             DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UINT,                DXGI_FORMAT_R8G8_UINT              },
    { FormatType::RG8_SINT,             DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_SINT,                DXGI_FORMAT_R8G8_SINT              },
    { FormatType::RG8_UNORM,            DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_UNORM,               DXGI_FORMAT_R8G8_UNORM             },
    { FormatType::RG8_SNORM,            DXGI_FORMAT_R8G8_TYPELESS,          DXGI_FORMAT_R8G8_SNORM,               DXGI_FORMAT_R8G8_SNORM             },
    { FormatType::R16_UINT,             DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UINT,                 DXGI_FORMAT_R16_UINT               },
    { FormatType::R16_SINT,             DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_SINT,                 DXGI_FORMAT_R16_SINT               },
    { FormatType::R16_UNORM,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_R16_UNORM              },
    { FormatType::R16_SNORM,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_SNORM,                DXGI_FORMAT_R16_SNORM              },
    { FormatType::R16_FLOAT,            DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_FLOAT,                DXGI_FORMAT_R16_FLOAT              },
    { FormatType::BGRA4_UNORM,          DXGI_FORMAT_B4G4R4A4_UNORM,         DXGI_FORMAT_B4G4R4A4_UNORM,           DXGI_FORMAT_B4G4R4A4_UNORM         },
    { FormatType::B5G6R5_UNORM,         DXGI_FORMAT_B5G6R5_UNORM,           DXGI_FORMAT_B5G6R5_UNORM,             DXGI_FORMAT_B5G6R5_UNORM           },
    { FormatType::B5G5R5A1_UNORM,       DXGI_FORMAT_B5G5R5A1_UNORM,         DXGI_FORMAT_B5G5R5A1_UNORM,           DXGI_FORMAT_B5G5R5A1_UNORM         },
    { FormatType::RGBA8_UINT,           DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UINT,            DXGI_FORMAT_R8G8B8A8_UINT          },
    { FormatType::RGBA8_SINT,           DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_SINT,            DXGI_FORMAT_R8G8B8A8_SINT          },
    { FormatType::RGBA8_UNORM,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM,           DXGI_FORMAT_R8G8B8A8_UNORM         },
    { FormatType::RGBA8_SNORM,          DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_SNORM,           DXGI_FORMAT_R8G8B8A8_SNORM         },
    { FormatType::BGRA8_UNORM,          DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM,           DXGI_FORMAT_B8G8R8A8_UNORM         },
    { FormatType::SRGBA8_UNORM,         DXGI_FORMAT_R8G8B8A8_TYPELESS,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,      DXGI_FORMAT_R8G8B8A8_UNORM_SRGB    },
    { FormatType::SBGRA8_UNORM,         DXGI_FORMAT_B8G8R8A8_TYPELESS,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB,      DXGI_FORMAT_B8G8R8A8_UNORM_SRGB    },
    { FormatType::R10G10B10A2_UNORM,    DXGI_FORMAT_R10G10B10A2_TYPELESS,   DXGI_FORMAT_R10G10B10A2_UNORM,        DXGI_FORMAT_R10G10B10A2_UNORM      },
    { FormatType::R11G11B10_FLOAT,      DXGI_FORMAT_R11G11B10_FLOAT,        DXGI_FORMAT_R11G11B10_FLOAT,          DXGI_FORMAT_R11G11B10_FLOAT        },
    { FormatType::RG16_UINT,            DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UINT,              DXGI_FORMAT_R16G16_UINT            },
    { FormatType::RG16_SINT,            DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_SINT,              DXGI_FORMAT_R16G16_SINT            },
    { FormatType::RG16_UNORM,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_UNORM,             DXGI_FORMAT_R16G16_UNORM           },
    { FormatType::RG16_SNORM,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_SNORM,             DXGI_FORMAT_R16G16_SNORM           },
    { FormatType::RG16_FLOAT,           DXGI_FORMAT_R16G16_TYPELESS,        DXGI_FORMAT_R16G16_FLOAT,             DXGI_FORMAT_R16G16_FLOAT           },
    { FormatType::R32_UINT,             DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_UINT,                 DXGI_FORMAT_R32_UINT               },
    { FormatType::R32_SINT,             DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_SINT,                 DXGI_FORMAT_R32_SINT               },
    { FormatType::R32_FLOAT,            DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_R32_FLOAT              },
    { FormatType::RGBA16_UINT,          DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UINT,        DXGI_FORMAT_R16G16B16A16_UINT      },
    { FormatType::RGBA16_SINT,          DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_SINT,        DXGI_FORMAT_R16G16B16A16_SINT      },
    { FormatType::RGBA16_FLOAT,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_FLOAT,       DXGI_FORMAT_R16G16B16A16_FLOAT     },
    { FormatType::RGBA16_UNORM,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_UNORM,       DXGI_FORMAT_R16G16B16A16_UNORM     },
    { FormatType::RGBA16_SNORM,         DXGI_FORMAT_R16G16B16A16_TYPELESS,  DXGI_FORMAT_R16G16B16A16_SNORM,       DXGI_FORMAT_R16G16B16A16_SNORM     },
    { FormatType::RG32_UINT,            DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_UINT,              DXGI_FORMAT_R32G32_UINT            },
    { FormatType::RG32_SINT,            DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_SINT,              DXGI_FORMAT_R32G32_SINT            },
    { FormatType::RG32_FLOAT,           DXGI_FORMAT_R32G32_TYPELESS,        DXGI_FORMAT_R32G32_FLOAT,             DXGI_FORMAT_R32G32_FLOAT           },
    { FormatType::RGB32_UINT,           DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_UINT,           DXGI_FORMAT_R32G32B32_UINT         },
    { FormatType::RGB32_SINT,           DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_SINT,           DXGI_FORMAT_R32G32B32_SINT         },
    { FormatType::RGB32_FLOAT,          DXGI_FORMAT_R32G32B32_TYPELESS,     DXGI_FORMAT_R32G32B32_FLOAT,          DXGI_FORMAT_R32G32B32_FLOAT        },
    { FormatType::RGBA32_UINT,          DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_UINT,        DXGI_FORMAT_R32G32B32A32_UINT      },
    { FormatType::RGBA32_SINT,          DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_SINT,        DXGI_FORMAT_R32G32B32A32_SINT      },
    { FormatType::RGBA32_FLOAT,         DXGI_FORMAT_R32G32B32A32_TYPELESS,  DXGI_FORMAT_R32G32B32A32_FLOAT,       DXGI_FORMAT_R32G32B32A32_FLOAT     },

    { FormatType::D16,                  DXGI_FORMAT_R16_TYPELESS,           DXGI_FORMAT_R16_UNORM,                DXGI_FORMAT_D16_UNORM              },
    { FormatType::D24S8,                DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_R24_UNORM_X8_TYPELESS,    DXGI_FORMAT_D24_UNORM_S8_UINT      },
    { FormatType::X24G8_UINT,           DXGI_FORMAT_R24G8_TYPELESS,         DXGI_FORMAT_X24_TYPELESS_G8_UINT,     DXGI_FORMAT_D24_UNORM_S8_UINT      },
    { FormatType::D32,                  DXGI_FORMAT_R32_TYPELESS,           DXGI_FORMAT_R32_FLOAT,                DXGI_FORMAT_D32_FLOAT              },
    { FormatType::D32S8,                DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS, DXGI_FORMAT_D32_FLOAT_S8X24_UINT   },
    { FormatType::X32G8_UINT,           DXGI_FORMAT_R32G8X24_TYPELESS,      DXGI_FORMAT_X32_TYPELESS_G8X24_UINT,  DXGI_FORMAT_D32_FLOAT_S8X24_UINT   },

    { FormatType::BC1_UNORM,            DXGI_FORMAT_BC1_TYPELESS,           DXGI_FORMAT_BC1_UNORM,                DXGI_FORMAT_BC1_UNORM              },
    { FormatType::BC1_UNORM_SRGB,       DXGI_FORMAT_BC1_TYPELESS,           DXGI_FORMAT_BC1_UNORM_SRGB,           DXGI_FORMAT_BC1_UNORM_SRGB         },
    { FormatType::BC2_UNORM,            DXGI_FORMAT_BC2_TYPELESS,           DXGI_FORMAT_BC2_UNORM,                DXGI_FORMAT_BC2_UNORM              },
    { FormatType::BC2_UNORM_SRGB,       DXGI_FORMAT_BC2_TYPELESS,           DXGI_FORMAT_BC2_UNORM_SRGB,           DXGI_FORMAT_BC2_UNORM_SRGB         },
    { FormatType::BC3_UNORM,            DXGI_FORMAT_BC3_TYPELESS,           DXGI_FORMAT_BC3_UNORM,                DXGI_FORMAT_BC3_UNORM              },
    { FormatType::BC3_UNORM_SRGB,       DXGI_FORMAT_BC3_TYPELESS,           DXGI_FORMAT_BC3_UNORM_SRGB,           DXGI_FORMAT_BC3_UNORM_SRGB         },
    { FormatType::BC4_UNORM,            DXGI_FORMAT_BC4_TYPELESS,           DXGI_FORMAT_BC4_UNORM,                DXGI_FORMAT_BC4_UNORM              },
    { FormatType::BC4_SNORM,            DXGI_FORMAT_BC4_TYPELESS,           DXGI_FORMAT_BC4_SNORM,                DXGI_FORMAT_BC4_SNORM              },
    { FormatType::BC5_UNORM,            DXGI_FORMAT_BC5_TYPELESS,           DXGI_FORMAT_BC5_UNORM,                DXGI_FORMAT_BC5_UNORM              },
    { FormatType::BC5_SNORM,            DXGI_FORMAT_BC5_TYPELESS,           DXGI_FORMAT_BC5_SNORM,                DXGI_FORMAT_BC5_SNORM              },
    { FormatType::BC6H_UFLOAT,          DXGI_FORMAT_BC6H_TYPELESS,          DXGI_FORMAT_BC6H_UF16,                DXGI_FORMAT_BC6H_UF16              },
    { FormatType::BC6H_SFLOAT,          DXGI_FORMAT_BC6H_TYPELESS,          DXGI_FORMAT_BC6H_SF16,                DXGI_FORMAT_BC6H_SF16              },
    { FormatType::BC7_UNORM,            DXGI_FORMAT_BC7_TYPELESS,           DXGI_FORMAT_BC7_UNORM,                DXGI_FORMAT_BC7_UNORM              },
    { FormatType::BC7_UNORM_SRGB,       DXGI_FORMAT_BC7_TYPELESS,           DXGI_FORMAT_BC7_UNORM_SRGB,           DXGI_FORMAT_BC7_UNORM_SRGB         },
};

const PhxEngine::RHI::Dx12::DxgiFormatMapping& PhxEngine::RHI::Dx12::GetDxgiFormatMapping(FormatType abstractFormat)
{
    static_assert(sizeof(c_FormatMappings) / sizeof(DxgiFormatMapping) == size_t(FormatType::COUNT),
        "The format mapping table doesn't have the right number of elements");

    const DxgiFormatMapping& mapping = c_FormatMappings[uint32_t(abstractFormat)];
    assert(mapping.abstractFormat == abstractFormat);
    return mapping;
}