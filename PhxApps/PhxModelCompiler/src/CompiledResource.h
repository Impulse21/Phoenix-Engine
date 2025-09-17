#pragma once

struct CompiledResource
{
    std::string name;
    std::string ext;

    // Keep metadata chunk separate as they are stored differently in pak files.
    phx::MemoryBuffer metadata_chunk;
    std::vector<phx::MemoryBuffer> chunks;
};