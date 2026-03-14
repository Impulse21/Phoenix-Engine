#pragma once

#include <PhxAsset/IAssetLoader.h>
#include <PhxAsset/AssetPtr.h>

#include <PhxCore/Reflect/TypeInfo.h>

#include <memory>

namespace phx::asset
{
    namespace AssetDB
    {
        void Initialize(std::unique_ptr<IAssetLoader> loader);

        void* Find(std::string_view name, const reflect::TypeInfo& type_info);

        template <typename TAsset>
        AssetPtr<TAsset> Get(std::string_view name)
        {
            // TODO: See if we can deduce the type from the name extension
            const reflect::TypeInfo* type_info = reflect::TypeRegistry::Find<TAsset>();
            PHX_ASSERT(type_info);
            void* ptr = Find(name, *type_info);

            return { static_cast<TAsset*>(ptr) };
        }

    }
}