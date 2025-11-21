#pragma once

#include <PhxRenderer/ShaderLibrary.h>

namespace phx::renderer
{
    void Initialize(ShaderLibraryDescriptor const& library_desc);
    void Shutdown();

}