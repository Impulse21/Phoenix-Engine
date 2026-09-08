#pragma once

#include <PhxEngine/Core/PhxDefines.h>
#include <PhxEngine/Core/MemoryBuffer.h>
#include <PhxEngine/Core/RelativePtr.h>
#include <PhxEngine/Core/RefCountPtr.h>

#include <PhxEngine/RHI/RHITypes.h>

#include "Resource.h"

namespace phx::resources
{
    struct MeshResource : public Resource
    {
        PHX_DECLARE_RESOURCE(MeshResource);

        struct CpuData
        {
            struct DrawInfo
            {
                // index_byte_offset/stream_header_byte_offset are relative
                // to the GPU chunk's base (== packed_mesh_buffer), not the
                // file or this CpuData -- usable as GPU byte offsets
                // straight out of the upload with no adjustment at draw
                // time.
                u32 index_count;
                u32 index_byte_offset;
                u32 stream_header_byte_offset;

                // Offset into the file's string table (see
                // ResourceFileHeader::strings_offset); 0 if this primitive
                // has no material. Not consumed yet -- material cooking is
                // out of scope for this pass -- but carried through so it
                // doesn't need re-threading later.
                u32 material_name_offset;
            };

            RelativePtr<DrawInfo> draw_info;
            u32 draw_info_count;
        };

        phx::MemoryBuffer cpu_data_buffer;   // owns the file's CPU chunk
        TypedView<CpuData> cpu_data;

        rhi::GpuAllocation packed_mesh_buffer;   // the file's GPU chunk, uploaded verbatim

        void Dispose() override;
    };

    // Interprets already-in-memory .phxmsh bytes (from SerializeMesh, or a
    // file read back off disk) and uploads its GPU chunk. `file_bytes` is
    // consumed -- it becomes the returned resource's cpu_data_buffer.
    RefCountPtr<MeshResource> CreateMeshResource(MemoryBuffer&& file_bytes);

    // VFS::ReadFile(virtual_path) + CreateMeshResource. Returns an invalid
    // (null) pointer on a read failure.
    RefCountPtr<MeshResource> LoadMeshResource(const char* virtual_path);
}