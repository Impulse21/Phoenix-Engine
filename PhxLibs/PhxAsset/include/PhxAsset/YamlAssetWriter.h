#pragma once

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxAsset/AssetExporter.h>

namespace YAML 
{
    class Emitter;
}

namespace phx::asset
{
    class YamlAssetWriter : public IAssetWriter
    {
    public:
        explicit YamlAssetWriter(IVirtualFileSystem* vfs);

         bool Write(std::string_view path, const reflect::TypeInfo& type_info, const void* asset) override;

    private:
        void WriteStruct(YAML::Emitter& emitter, const reflect::TypeInfo& type_info, const void* asset);
        void WriteField(YAML::Emitter& emitter, const reflect::FieldInfo& field_info, const void* field_ptr);
        void WriteArray(std::ostream& out, const reflect::FieldInfo& field_info, const void* vec_ptr);
        
    private:
        IVirtualFileSystem* m_vfs;
    };
}