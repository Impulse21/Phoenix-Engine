#pragma once

#include "ResourceFileFormat.h"

namespace phx::resources
{
    constexpr u32 kMeshFileMagic   = MakeTag('P', 'X', 'M', 'S');
    constexpr u16 kMeshFileVersion = 1;

    constexpr u32 kMeshChunkType_Cpu = MakeTag('C', 'P', 'U', '\0');
    constexpr u32 kMeshChunkType_Gpu = MakeTag('G', 'P', 'U', '\0');
}
