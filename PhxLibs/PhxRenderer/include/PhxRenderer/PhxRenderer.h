#pragma once

#include <PhxRenderer/ShaderLIbrary.h>

namespace phx::renderer
{
    void Initialize(ShaderLibraryDescriptor const& library_desc);
    void Shutdown();

}