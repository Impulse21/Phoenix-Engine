#include "PhxRenderer_pch.h"

#include <PhxRenderer/Shaders/ShaderModuleResource.h>

using namespace phx;
using namespace phx::renderer;

bool phx::renderer::ShaderModuleResource::FindStruct(const std::string& name, ShaderStructDesc& out_desc)
{
    auto emplace_result = types_descs.try_emplace(name, [&]()
    {
        slang::ProgramLayout* program_layout = slang_module->getLayout();
        PHX_ASSERT(program_layout);

        slang::TypeReflection* type_reflection = program_layout->findTypeByName(name.c_str());
        if (!type_reflection)
            return false;

        // TODO: Cache this

        slang::TypeLayoutReflection* type_layout = program_layout->getTypeLayout(type_reflection, slang::LayoutRules::Default);

        ShaderStructDesc desc = {
            .name = StringHash(name.c_str()),
            .size = (uint32_t)type_layout->getSize(),
        };
        
        desc.fields.resize(type_reflection->getFieldCount());
        for (uint32_t i = 0; i < type_reflection->getFieldCount(); ++i)
        {
            auto field_reflection = type_reflection->getFieldByIndex(i);
            auto field_layout = type_layout->getFieldByIndex(i);

            desc.fields[i] = {
                .name = StringHash(field_reflection->getName()),
                .offset = static_cast<uint32_t>(field_layout->getOffset()),
                .size = static_cast<uint32_t>(field_layout->getSize()),
            };
        }
    });
    
    out_desc = emplace_result.first->second;
    return emplace_result.second;
}