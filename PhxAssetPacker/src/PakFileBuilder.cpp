#include "PakFileBuilder.h"

#include <PhxCore/VFS.h>
#include <PhxCore/BinaryBuilder.h>
#include <PhxCore/IO/PakFile.h>

using namespace phx;

std::unique_ptr<IBlob> phx::PakFileBuilder::Build()
{
    // Build THe pack file
    BinaryBuilder packFileBuilder;
    OffsetHandle headerOffset = packFileBuilder.Reserve<PakFileFormat::Header>();

    // TODO: I am here.
    return std::unique_ptr<IBlob>();
}
