#pragma once

#include "ResourceFileFormat.h"

namespace phx::resources
{
    constexpr u32 kMaterialFileMagic     = MakeTag('P', 'X', 'M', 'T');
    constexpr u16 kMaterialFileVersion   = 1;

    constexpr u32 kMaterialChunkType_Cpu = MakeTag('C', 'P', 'U', '\0');
}
