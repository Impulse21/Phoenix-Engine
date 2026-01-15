#pragma once

#include <PhxCore/RefCountPtr.h>

#include <PhxRenderer/shaders/ShaderModuleResource.h>

#include <slang.h>
#include <slang-com-ptr.h>

namespace phx::renderer
{
    struct ShaderEntryPoint
    {
        std::string name;
        rhi::ShaderStage stage;
    };

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


    struct ShaderCompilerCreateInfo
    {
        rhi::ShaderFormat target = rhi::ShaderFormat::Spirv;
        std::vector<std::string> include_paths;

        std::vector<std::pair<std::string, std::string>> defines;

        bool save_debug_symbols = false;
        bool debug_info         = false;
        bool optimization       = true;
        bool warning_as_errors  = true;
        bool force_column_major = false;
    };

    struct ShaderCompileDescriptor
    {
        ShaderModuleResource* shader_module_resource;
        struct GenericArg
        {
            std::string name;
            std::string value;

            bool is_type = false;
        };

        std::vector<GenericArg> generic_args;
        Span<ShaderEntryPoint> entry_points;

        Hash64 GetHash() const;
    };

    class SlangShaderCompiler
    {
    public:
        static void Initialize(const ShaderCompilerCreateInfo& create_info)
        {
			ms_create_info = create_info;
            InitializeSlang();
        }

		static Slang::ComPtr<slang::IModule> LoadModule(
            const void* slang_data,
            size_t slang_data_size,
            const char* module_name,
            const char* path);

        static Slang::ComPtr<slang::IModule> LoadModule(const std::string& path, const std::string& source);
        static RefCountPtr<SlangShader> Compile(const ShaderCompileDescriptor& compile_desc);

    private:
        static void InitializeSlang();

    private:
        inline static ShaderCompilerCreateInfo ms_create_info;
        inline static std::mutex ms_mutex;
        inline static Slang::ComPtr<slang::IGlobalSession> ms_global_session;
        inline static Slang::ComPtr<slang::ISession> ms_session; // Not thread safe

    };
}

