#pragma once

#include <string>
namespace phx::reflect
{
    struct TypeInfo;
}

namespace phx::asset
{
    class IAssetLoader
    {
    public:
        virtual bool Load(std::string_view path, const reflect::TypeInfo& type_info, void* out) const= 0;
        virtual bool Exists(std::string_view path) const = 0;
        virtual bool UseCache() const = 0
        virtual ~IAssetLoader() = default;
    };
}