#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/Hash.h>
#include <PhxRhi/PhxRhi.h>
#include <string>

#include <vector>
#include <unordered_map>

#include <slang.h>
#include <slang-com-ptr.h>

namespace phx::renderer
{
    struct ShaderEntryPoint
    {
        std::string name;
        rhi::ShaderStage stage;
    };
    class SlangShader : public RefCounted
    {
    public:
        const void* GetByteCode() const { return m_code_blob->getBufferPointer(); }
        size_t GetByteCodeSize() const { return m_code_blob->getBufferSize(); }

        slang::ProgramLayout* GetReflection() const { return m_linked_programs ? m_linked_programs->getLayout() : nullptr; }

        const void* GetEntryPointCode(int entryPointIndex, size_t& outSize) const;
        const char* GetEntryPoint(rhi::ShaderStage stage);

        rhi::ShaderModuleHandle GetShaderModule() const { return m_shader_module; }

        ~SlangShader();

    private:
        friend class ShaderLibrary;

    private:
        Slang::ComPtr<slang::IComponentType> m_linked_programs;
        Slang::ComPtr<slang::IBlob> m_code_blob;
        std::vector<ShaderEntryPoint> m_entry_points;
        rhi::ShaderModuleHandle m_shader_module;
    };

	class ShaderAsset : public RefCounted
	{
	public:
        RefCountPtr<SlangShader> Get() const { return m_current; }

    private:
        friend class ShaderLibrary;
        RefCountPtr<SlangShader> m_current;
        std::string m_src_path;
	};
        
    struct ShaderCompileDescriptor
    {
        std::string source_file_path;
        struct GenericArg
        {
            std::string name;
            std::string value;

            bool is_type= false;
        };

        std::vector<GenericArg> generic_args;
        Span<ShaderEntryPoint> entry_points;

        Hash64 GetHash() const;
    };

    struct ShaderLibraryDescriptor
    {
        rhi::ShaderFormat target = rhi::ShaderFormat::Spirv;
        std::vector<std::string> include_paths;

        std::vector<std::pair<std::string, std::string>> defines;

        bool debug_info = false;
        bool optimization = true;
        bool warning_as_errors = true;
        bool ForceColumnMajor = false;
    };

	class ShaderLibrary
	{
	public:
		inline static ShaderLibrary* Ptr = nullptr;

	public:
		void Initialize(const ShaderLibraryDescriptor& librar_desc);
		void Shutdown();

        RefCountPtr<ShaderAsset> LoadShader(ShaderCompileDescriptor const& compile_desc);

        void ReloadAll();

    private:
        RefCountPtr<SlangShader> Compile(ShaderCompileDescriptor const& compile_desc);
        void ConstructSession();

    private:
        ShaderLibraryDescriptor m_library_desc;

        Slang::ComPtr<slang::IGlobalSession> m_global_session;
        Slang::ComPtr<slang::ISession> m_session; // Not thread safe

        std::unordered_map<Hash64, RefCountPtr<ShaderAsset>> m_cached_assets;
        std::unordered_map<Hash64, ShaderCompileDescriptor> m_cached_compile_desc;
        std::mutex m_cache_mutex;

	};
}