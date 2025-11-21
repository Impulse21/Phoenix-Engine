#pragma once

#include <PhxCore/RefCountPtr.h>
#include <PhxCore/Hash.h>
#include <PhxRhi/RHICommon.h>
#include <string>

#include <vector>
#include <unordered_map>

#include <slang.h>
#include <slang-com-ptr.h>

namespace phx::renderer
{
    class SlangShader : public RefCounted
    {
    public:
        // The Raw Bytecode for the RHI (Vulkan/DX12)
        const void* GetByteCode() const;
        size_t GetByteCodeSize() const;

        // The Reflection Data for the Material System
        slang::ProgramLayout* GetReflection() const;

        // The Entry Point code (sometimes needed separately for Vulkan)
        // If entryPointIndex is -1, returns the composite blob.
        const void* GetEntryPointCode(int entryPointIndex, size_t& outSize) const;

    private:
        friend class ShaderLibrary;

    private:
        // We hold these to keep the pointers valid
        Slang::ComPtr<slang::IComponentType> m_linked_programs;
        Slang::ComPtr<slang::IBlob> m_code_blob;
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
        std::vector<std::pair<std::string, std::string>> defines;

        struct EntryPoint
        {
            std::string name;
            rhi::ShaderStage stage;
        };
        std::vector<EntryPoint> entry_points;

        rhi::ShaderFormat target = rhi::ShaderFormat::Spirv;

        bool debug_info = false;
        bool optimization = true;
        bool warning_as_errors = true; 

        Hash64 GetHash() const;
    };

	class ShaderLibrary
	{
	public:
		inline static ShaderLibrary* Ptr = nullptr;

	public:
		void Initialize(std::vector<std::string>&& include_paths);
		void Shutdown();

        RefCountPtr<ShaderAsset> LoadShader(ShaderCompileDescriptor const& compile_desc);

        void ReloadAll();

    private:
        RefCountPtr<SlangShader> Compile(ShaderCompileDescriptor const& compile_desc);

    private:
        Slang::ComPtr<slang::IGlobalSession> m_global_session;
        Slang::ComPtr<slang::ISession> m_session; // Not thread safe

        std::unordered_map<Hash64, RefCountPtr<ShaderAsset>> m_cached_assets;
        std::unordered_map<Hash64, ShaderCompileDescriptor> m_cached_compile_desc;
        std::mutex m_cache_mutex;

        std::vector<std::string> m_include_paths;

	};
}