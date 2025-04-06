#pragma once
#include <unordered_map>
#include <string>
#include <cstddef> // For std::size_t

namespace phx::rfl 
{

struct PropertyInfo 
{
    const char* name;
    const char* tooltip;
    const char* typeName;
    std::size_t offset;
};

struct StructInfo 
{
    const char* name;
    std::unordered_map<std::string, PropertyInfo> properties;
};

extern std::unordered_map<std::string, StructInfo> g_metadata;

} // namespace Reflection

