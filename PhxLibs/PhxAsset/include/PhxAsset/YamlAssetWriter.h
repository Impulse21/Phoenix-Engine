#pragma once

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxAsset/AssetExporter.h>

namespace phx::asset
{
    class YamlAssetWriter : public IAssetWriter
    {
    public:
        explicit YamlAssetWriter(IVirtualFileSystem* vfs);

         bool Write(std::string_view path, const reflect::TypeInfo& type_info, const void* asset) override;

    private:
        void WriteStruct(std::ostream& out, const reflect::TypeInfo& type_info, const void* asset);
        void WriteArray(std::ostream& out, const reflect::TypeInfo& type_info, const void* vec_ptr, int indent);
        
    private:
        IVirtualFileSystem* m_vfs;
    };
}