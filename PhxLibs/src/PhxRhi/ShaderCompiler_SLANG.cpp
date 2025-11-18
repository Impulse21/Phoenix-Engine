#include "PhxRhi_pch.h"

#include <PhxCore/IVirtualFileSystem.h>

#include "ShaderCompiler.h"

#include <slang.h>
#include <slang-com-ptr.h>

namespace phx::rhi
{

	namespace SlangCompiler
	{
		Result<ShaderCompiler::Output, std::string> Compile(IVirtualFileSystem* vfs, ShaderCompiler::Input const& input);
	}

	namespace ShaderCompiler
	{
		Result<Output, std::string> Compile(IVirtualFileSystem* vfs, Input const& input)
		{
			return SlangCompiler::Compile(vfs, input);
		}
	}
}


using namespace phx;
using namespace phx::rhi;
using namespace phx::rhi::ShaderCompiler;

namespace
{
	Slang::ComPtr<slang::IGlobalSession> g_slang_global_session;
    std::once_flag g_slang_init_flag;

	void EnsureSlangInitialized()
	{
        std::call_once(g_slang_init_flag, []() {
            slang::createGlobalSession(g_slang_global_session.writeRef());
        });
    }

#if false
    PushConstantReflection TranslatePushConstants(slang::ProgramLayout* layout)
    {
        using namespace phx::rhi::ShaderCompiler;
        PushConstantReflection outDesc;

        slang::ParameterCategory* range = layout->getPushConstantRange();
        if (!range || range->getBindingCount() == 0)
            return outDesc;

        slang::TypeLayout* typeLayout = range->getTypeLayout();
        outDesc.TotalSize = (uint32_t)typeLayout->getSize();
        outDesc.IsValid = true;

        // If it's a struct, iterate fields
        if (typeLayout->getKind() == slang::TypeKind::Struct)
        {
            for (SlangUInt i = 0; i < typeLayout->getFieldCount(); ++i)
            {
                auto field = typeLayout->getField(i);
                outDesc.Members.push_back({
                    field->getName(),
                    (uint32_t)field->getOffset(),
                    (uint32_t)field->getTypeLayout()->getSize()
                    });
            }
        }
        return outDesc;
    }

    std::vector<phx::rhi::ShaderCompiler::BindlessStructReflection> TranslateStructuredBuffers(slang::ProgramLayout* layout)
    {
        using namespace phx::rhi::ShaderCompiler;
        std::vector<BindlessStructReflection> outBuffers;

        unsigned paramCount = layout->getParameterCount();
        for (unsigned i = 0; i < paramCount; ++i)
        {
            auto param = layout->getParameter(i);
            auto type = param->getTypeLayout();

            // We are looking for generic buffers or structured buffers
            if (type->getKind() == slang::TypeKind::Resource)
            {
                // Inspect the element type
                auto elementType = type->getElementTypeLayout();
                if (elementType && elementType->getKind() == slang::TypeKind::Struct)
                {
                    BindlessStructReflection buff;
                    buff.BufferName = param->getName();
                    buff.Stride = (uint32_t)elementType->getSize();

                    for (SlangUInt f = 0; f < elementType->getFieldCount(); ++f)
                    {
                        auto field = elementType->getField(f);
                        buff.Members.push_back({
                            field->getName(),
                            (uint32_t)field->getOffset(),
                            (uint32_t)field->getTypeLayout()->getSize()
                            });
                    }
                    outBuffers.push_back(buff);
                }
            }
        }
        return outBuffers;
    }
#endif
    std::string GetStringFromBlob(slang::IBlob* blob)
    {
        if (!blob) return "";
        // Slang blobs are not guaranteed to be null-terminated, so we use the size.
        return std::string(
            (const char*)blob->getBufferPointer(),
            (size_t)blob->getBufferSize()
        );
    }
}

Result<Output, std::string> SlangCompiler::Compile(IVirtualFileSystem* vfs,Input const& input)
{
    EnsureSlangInitialized();

    Output output = {};
    slang::SessionDesc session_desc = {};

    session_desc.searchPaths = input.include_dir.data();
    session_desc.searchPathCount = (SlangInt)input.include_dir.size();

    std::vector<slang::PreprocessorMacroDesc> slang_macros;
    slang_macros.reserve(input.defines.size());

    for (auto& def : input.defines)
        slang_macros.push_back({ def, "1" });

    session_desc.preprocessorMacros = slang_macros.data();
    session_desc.preprocessorMacroCount = slang_macros.size();

    slang::TargetDesc target_desc = {};
    {
        switch (input.format)
        {
        case ShaderFormat::Spirv:
            target_desc.format = SLANG_SPIRV;
            target_desc.flags |= SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
            break;
        case ShaderFormat::Hlsl6:
            target_desc.format = SLANG_DXIL;
            break;
        default:
            return make_unexpected<std::string>("Invalid Shader Format specified.");
        }

        std::vector<slang::CompilerOptionEntry> compiler_option_entries;
        
        // -- Optimization ---
        {
            slang::CompilerOptionEntry opt = {};
            opt.name = slang::CompilerOptionName::Optimization;

            if (input.disable_optimizations)
                opt.value.intValue0 = SLANG_OPTIMIZATION_LEVEL_NONE;
            else
                opt.value.intValue0 = SLANG_OPTIMIZATION_LEVEL_HIGH; // or Maximal

            opt.value.kind = slang::CompilerOptionValueKind::Int;
            compiler_option_entries.push_back(opt);
        }

        //  -- Debug Info Option ---
        if (input.generate_debug_info)
        {
            slang::CompilerOptionEntry debugOpt = {};
            debugOpt.name = slang::CompilerOptionName::DebugInformation;
            debugOpt.value.intValue0 = SLANG_DEBUG_INFO_LEVEL_STANDARD; // or Full
            debugOpt.value.kind = slang::CompilerOptionValueKind::Int;
            compiler_option_entries.push_back(debugOpt);
        }

        target_desc.compilerOptionEntries = compiler_option_entries.data();
        target_desc.compilerOptionEntryCount = compiler_option_entries.size();

        const char* profile_name = "sm_6_6"; // Default fallback
        switch (input.shader_model)
        {
        case ShaderModel::SM_6_5: profile_name = "sm_6_5"; break;
        case ShaderModel::SM_6_6: profile_name = "sm_6_6"; break;
        case ShaderModel::SM_6_7: profile_name = "sm_6_7"; break;
        }
        target_desc.profile = g_slang_global_session->findProfile(profile_name);

        session_desc.targets = &target_desc;
        session_desc.targetCount = 1;
    }

    Slang::ComPtr<slang::ISession> session;
    g_slang_global_session->createSession(session_desc, session.writeRef());

    phx::Result<AsyncResourceDescriptor> descriptor_result = vfs->GetResourceDescriptorForAsync(input.source_filename);
    if (!descriptor_result)
    {
        return make_unexpected<std::string>("Failed to get file descriptor info");
    }

    Slang::ComPtr<slang::IBlob> diagnostic_blob;
    slang::IModule* module = session->loadModule(input.source_filename.c_str(), diagnostic_blob.writeRef());
    if (!module)
    {
        return make_unexpected<std::string>(GetStringFromBlob(diagnostic_blob));
    }

    // 3. Link Entry Points
    std::vector<slang::IComponentType*> components;
    slang::ComPtr<slang::IEntryPoint> vsEntry, psEntry;

    if (!input.VSEntryPoint.empty()) {
        module->findEntryPointByName(input.VSEntryPoint.c_str(), vsEntry.writeRef());
        if (vsEntry) components.push_back(vsEntry);
    }
    if (!input.PSEntryPoint.empty()) {
        module->findEntryPointByName(input.PSEntryPoint.c_str(), psEntry.writeRef());
        if (psEntry) components.push_back(psEntry);
    }

    slang::ComPtr<slang::IComponentType> linkedProgram;
    linkedProgram = session->createCompositeComponentType(
        components.data(), components.size(), diagnostics.writeRef());

    if (!linkedProgram)
    {
        output.ErrorMessage = diagnostics.getBuffer();
        return output;
    }

    // 4. Extract ByteCode
    slang::ComPtr<slang::IBlob> blob;
    linkedProgram->getEntryPointCode(0, 0, blob.writeRef(), diagnostics.writeRef());
    if (blob)
    {
        output.ByteCode.resize(blob->getBufferSize());
        memcpy(output.ByteCode.data(), blob->getBufferPointer(), blob->getBufferSize());
    }

    // 5. Translate Reflection (The Anti-Leak Magic)
    slang::ProgramLayout* slLayout = linkedProgram->getLayout();

    output.Reflection.PushConstants = TranslatePushConstants(slLayout);
    output.Reflection.StructuredBuffers = TranslateStructuredBuffers(slLayout);

    // 6. Extract Dependencies
    auto slangModule = module->getModule();
    for (int i = 0; i < slangModule->getDependencyFileCount(); ++i)
    {
        output.Dependencies.push_back(slangModule->getDependencyFilePath(i));
    }

    output.Success = true;
    return output;
}