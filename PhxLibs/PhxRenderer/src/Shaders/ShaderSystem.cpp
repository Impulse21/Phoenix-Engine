#include "PhxRenderer_pch.h"

#include <PhxCore/IVirtualFileSystem.h>
#include <PhxRhi/PhxRhi_Types.h>

#include <PhxRenderer/Shaders/ShaderSystem.h>
#include "SlangShaderCompiler.h"

#include <PhxCore/Pool.h>
#include <PhxCore/StringHash.h>

#include <optional>
#include "ShaderSystem.h"

using namespace phx;
using namespace phx::renderer;

namespace
{
    struct SlangShader
    {
        std::string source_path;
        Slang::ComPtr<slang::IModule> slang_module;
    };

    // -- Not thread safe
    std::optional<SlangCompilerSession> g_session;
}

void ShaderSystem::Initialize(Span<std::string> preload_list)
{
    SlangCompiler::Initialize();

    SlangCompilerSession session = SlangCompiler::CreateCompileSession({
        .target = rhi::ShaderFormat::Spirv,
        .include_paths = {},
        .defines = {},
    });

    // TODO: Check if valid.
    g_session = session;
}

void ShaderSystem::Shutdown()
{
    g_session.reset();
    SlangCompiler::Shutdown();
}

void ShaderSystem::RegisterModule(const std::string& name, const std::string& path)
{
    auto vfs = phx::IVirtualFileSystem::Ptr;

    FilePtr file = vfs->Open(path, FileMode::Read);
    if (!file)
    {
        PHX_CORE_ERROR("AssetDB: failed to open '{0}'", virtual_path);
        return;
    }

    std::string content(file->GetSize(), '\0');
    file->Read(content.data(), content.size());

    RegisterModule(name, path, content.data(), content.size());
}

void phx::renderer::ShaderSystem::RegisterModule(const std::string& name, const std::string& path, const void *data, size_t data_size)
{
	ISlangBlob* slang_shader_blob = slang_createBlob(data, data_size);
	Slang::ComPtr<slang::IBlob> diagnostic_blob;
	Slang::ComPtr<slang::IModule> shader_module = nullptr;

	{
		// const char* module_compiler_version;
		const char* module_name = "";
		shader_module = ms_session->loadModuleFromSource(
			module_name,
			path,
			slang_shader_blob,
			diagnostic_blob.writeRef());
	}

	if (!shader_module)
	{
		LogSlangDiagnostics(diagnostic_blob, path);
	}

	slang_shader_blob->Release();
	return shader_module;
}

ShaderHandle ShaderSystem::RequestShaderProgram(std::string virtual_path) 
{
}
