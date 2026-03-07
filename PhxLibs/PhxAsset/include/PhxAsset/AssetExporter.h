#pragma once

#include <PhxCore/Assert.h>

#include <string>
#include <TypeInfo.h>

namespace phx::asset
{
    class IAssetWriter
    {
    public:
        virtual bool Write(std::string_view path, const reflect::TypeInfo& type_info, const void* asset) = 0;
        virtual ~IAssetWriter() = default;
    };

    template<typename T>
    class AssetExporter
    {
    public:
        AssetExporter(IAssetWriter& writer)
            : m_writer(writer)
        {};

        bool Save(std::string_view path, const T& asset)
        {
            const auto* type_info = reflect::TypeRegistry<T>();
            PHX_ASSERT(type_info);
            return m_writer.Write(path, *type_info, &asset);
        }
        
    private:
        IAssetWriter& m_writer;
    }
}