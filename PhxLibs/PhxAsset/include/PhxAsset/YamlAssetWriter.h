#pragma once

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxAsset/AssetExporter.h>

namespace YAML 
{
    class Emitter;
}

namespace phx::asset
{

#if USE_VENDOR_REFLECTION

    class YamlAssetWriter : public IAssetWriter
    {
    public:
        explicit YamlAssetWriter(IVirtualFileSystem* vfs);

         bool Write(std::string_view path, const reflect::TypeInfo& type_info, const void* asset) override;
  
    private:
        IVirtualFileSystem* m_vfs;
    };
    
#else
    class YamlAssetWriter : public IAssetWriter
    {
    public:
        explicit YamlAssetWriter(IVirtualFileSystem* vfs);

         bool Write(std::string_view path, const reflect::TypeInfo& type_info, const void* asset) override;

    private:
        void WriteStruct(YAML::Emitter& emitter, const reflect::TypeInfo& type_info, const void* struct_ptr) const;
        void WriteField(YAML::Emitter& emitter, const reflect::FieldInfo& field_info, const void* field_ptr) const;
        void WriteArray(YAML::Emitter& emitter, const reflect::FieldInfo& field_info, const void* vec_ptr) const;
        
    private:
        IVirtualFileSystem* m_vfs;
    };
#endif
}