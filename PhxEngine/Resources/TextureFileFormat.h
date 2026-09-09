#pragma once

#include "ResourceFileFormat.h"

namespace phx::resources
{
    constexpr u32 kTextureFileMagic   = MakeTag('P', 'X', 'T', 'X');
    constexpr u16 kTextureFileVersion = 1;

    constexpr u32 kTextureChunkType_Cpu = MakeTag('C', 'P', 'U', '\0');
    constexpr u32 kTextureChunkType_Gpu = MakeTag('G', 'P', 'U', '\0');
}
