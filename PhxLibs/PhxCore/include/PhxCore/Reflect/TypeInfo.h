#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <hlsl++.h>

namespace phx::reflect
{
    enum class FieldKind : uint8_t
    {
        Bool,
        Int32, Int64,
        Uint32, Uint64,
        Float, Double,
        String,
        Float2, Float3, Float4,
        Nested,
        Array,
        AssetPtr,
    };

    struct TypeInfo;

    struct FieldInfo
    {
        std::string_view    name;
        FieldKind           kind;
        size_t              offset;
        size_t              size;   

        const TypeInfo*     nested_type = nullptr;
        size_t              element_size = 0;

        template<class T>
        T& Get(void* obj)
        {
            return *reinterpret_cast<T*>(static_cast<uint8_t*>(obj) + offset);
        }

        template<class T>
        const T& Get(void* obj) const
        {
            return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(obj) + offset);
        }
    };

    struct TypeInfo
    {
        std::string_view        name;
        size_t                  size;
        std::vector<FieldInfo>  fields;

        void* (*construct)();
        void (*destruct)(void*);
        void (*construct_place)(void*);
        void (*destruct_place)(void*);
        const FieldInfo* FindField(std::string_view field_name) const
        {
            for (const auto& field : fields)
            {
                if (field.name == field_name)
                    return &field;
            }

            return nullptr;
        }
    };

    namespace TypeRegistry
    {
        namespace Detail
        {
            inline std::unordered_map<std::string, TypeInfo>& Store()
            {
                static std::unordered_map<std::string, TypeInfo> s;
                return s;
            }
        }

        inline void RegisterType(std::string_view name, TypeInfo&& type_info)
        {
            Detail::Store()[std::string(name)] = std::move(type_info);
        }

        inline const TypeInfo* Find(std::string_view name) 
        {
            auto& store  = Detail::Store();
            auto  it = store.find(std::string(name));
            return it != store.end() ? &it->second : nullptr;
        }

        template<typename T>
        inline const TypeInfo* Find()
        {
            return Find(T::GetTypeNameStatic());
        }
    }

    template<typename T> struct KindOf;
    template<> struct KindOf<bool>        { static constexpr FieldKind value = FieldKind::Bool;   };
    template<> struct KindOf<int32_t>     { static constexpr FieldKind value = FieldKind::Int32;  };
    template<> struct KindOf<int64_t>     { static constexpr FieldKind value = FieldKind::Int64;  };
    template<> struct KindOf<uint32_t>    { static constexpr FieldKind value = FieldKind::Uint32; };
    template<> struct KindOf<uint64_t>    { static constexpr FieldKind value = FieldKind::Uint64; };
    template<> struct KindOf<float>       { static constexpr FieldKind value = FieldKind::Float;  };
    template<> struct KindOf<double>      { static constexpr FieldKind value = FieldKind::Double; };
    template<> struct KindOf<std::string> { static constexpr FieldKind value = FieldKind::String; };

    // hlsl++
    template<> struct KindOf<hlslpp::interop::float2> { static constexpr FieldKind value = FieldKind::Float2; };
    template<> struct KindOf<hlslpp::interop::float3> { static constexpr FieldKind value = FieldKind::Float3; };
    template<> struct KindOf<hlslpp::interop::float4> { static constexpr FieldKind value = FieldKind::Float4; };
}
