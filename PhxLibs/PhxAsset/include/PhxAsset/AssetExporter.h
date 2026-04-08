#pragma once

#include <PhxCore/Assert.h>

#include <rfl.hpp>
#include <rfl/yaml.hpp>

#include <string>

#include <PhxCore/UUID.h>
#include <PhxAsset/ReflectCppBindings.h>
#include <PhxCore/Reflect/Reflection.h>

namespace phx::asset
{
    template<typename TAsset>
    class AssetExporter
    {
    public:
        static bool Export(const char* path, const TAsset& asset)
        {
            AssetExporter<TAsset> exporter;
            return exporter.Save(path, asset);
        }
        
    public:
        AssetExporter() = default;

        bool Save(const char* path, const TAsset& asset)
        {
            std::ofstream out(path);
            rfl::yaml::write<TAsset>(asset, out);

            return true;
        }
        
    private:
    };
}