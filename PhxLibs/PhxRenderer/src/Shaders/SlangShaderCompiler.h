#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/Span.h>
#include <PhxCore/IVirtualFileSystem.h>

#include <PhxRenderer/Shaders/ShaderSystemTypes.h>

#include <slang.h>
#include <slang-com-ptr.h>

#include <unordered_map>

namespace phx::renderer
{
    struct SlangShader : public RefCounted
    {
        Slang::ComPtr<slang::IComponentType>    linked_programs;
        Slang::ComPtr<slang::IBlob>             code_blob;
        std::vector<ShaderEntryPoint>           entry_points;

		// -- Helpers ---
        const void* GetByteCode() const { return code_blob->getBufferPointer(); }
        size_t GetByteCodeSize() const { return code_blob->getBufferSize(); }

        slang::ProgramLayout* GetReflection() const { return linked_programs ? linked_programs->getLayout() : nullptr; }

        const void* GetEntryPointCode(int entryPointIndex, size_t& outSize) const;
        const char* GetEntryPoint(rhi::ShaderStage stage);
    };

    struct SlangCompilerSessionDescriptor
    {
        phx::IVirtualFileSystem*    vfs;
        rhi::ShaderFormat           target = rhi::ShaderFormat::Spirv;
        std::vector<std::string>    include_paths;

        std::vector<std::pair<std::string, std::string>> defines;

        bool save_debug_symbols = false;
        bool debug_info         = false;
        bool optimization       = true;
        bool warning_as_errors  = true;
        bool force_column_major = false;
    };

    class SlangCompilerSession
    {
    public:
        SlangCompilerSession(
            const slang::SessionDesc& slang_desc,
            Slang::ComPtr<slang::IGlobalSession> global_session,
            IVirtualFileSystem* vfs);

        void Initialize();

        Slang::ComPtr<slang::IModule> LoadModule(const std::string& virtual_path);
        Slang::ComPtr<slang::IModule> LoadModule(
            const std::string& module_name,
            const std::string& path,
            const void* data,
            size_t data_size);


        RefCountPtr<SlangShader> LinkProgram(const ShaderDescriptor shader_Desc);

    private:
        IVirtualFileSystem* m_vfs;
        std::unordered_map<std::string, Slang::ComPtr<slang::IModule>> m_loaded_modules;
        slang::SessionDesc m_slang_desc;
        Slang::ComPtr<slang::ISession> m_slang_session;
        Slang::ComPtr<slang::IGlobalSession> m_global_session;
    };

    namespace SlangCompiler
    {
        void Initialize();
        void Shutdown ();

        SlangCompilerSession* GetOrCreateCompilerSession();

        // TODO Depericate?
        std::unique_ptr<SlangCompilerSession> CreateCompileSession(const SlangCompilerSessionDescriptor& desc);
    }
}

