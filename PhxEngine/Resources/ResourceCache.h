#pragma once

#include "ResourceFileFormat.h"

namespace phx::resources
{
    bool IsCookedFileUpToDate(const char* virtual_path, u32 magic, u16 version, u64 source_hash);
}
