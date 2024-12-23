require("premake/phxengine/dependencies")

-- Directories
engine_directory            = 'phxengine'
engine_include_directory    = engine_directory..'/include'        
engine_src_directory        = engine_directory..'/src'

editor_directory            = 'phxeditor'
editor_src_directory        = editor_directory..'/src'

workspace_directory         = '.workspace/'.._ACTION


-- IDE Platform Names
msvc_win_64                 = 'MSVC Win64'
clang_win_64                = 'CLang Win64'

platform_windows            = "Windows"
rhi_vulkan                  = "Vulkan"
rhi_d3d12                   = "D3D12"

selected_platform           = nil
selected_rhi                = nil

win_64_platform_filters     = "platforms:"..msvc_win_64..' or '..clang_win_64


-- Project Names
project_name_phx_engine     = 'PhxEngine'
project_name_phx_editor     = 'PhxEditor'


-- Generated Code Directories
generated_shader_directory  = workspace_directory..'/GeneratedShaders'
generated_code_directory    = workspace_directory..'/GeneratedCode'

--ShaderCompilerExecutableName = ProjectShaderCompiler..'.exe'
-- Path is resolved during the parsing pass. It's meant to be in the same directory as the main executable
--ShaderCompilerPath = '%{cfg.buildtarget.directory}/'..ShaderCompilerExecutableName

is_visual_studio = string.sub(_ACTION, 1, string.len('vs')) == 'vs'
visual_studio_version = ''
if is_visual_studio then
	visual_studio_version = string.sub(_ACTION, string.len('vs') + 1)
end

-- Utility Functions

function ExcludePlatformSpecificCode(rootPath)
	excludes { rootPath..'**/platform/**' }
end

function ExcludePlatformSpecificRHI(rootPath)
	excludes { rootPath..'**/rhi/dx12/**' }
	excludes { rootPath..'**/rhi/vulkan/**' }
end

function CopyFileCommand(filePath, destinationPath)
	return '{copyfile} "'..filePath..'" "'..destinationPath..'"'
end

function MakeDirCommand(directoryPath)
	return '{mkdir} "'..directoryPath..'"'
end

-- Add warnings globally to fix serious issues that could cause incorrect
-- runtime behavior or crashes.
-- Remove warnings globally only if it makes sense to do so. For example,
-- we are interested in using a specific extension or feature.
function HandleGlobalWarnings()

	filter('toolset:msc*')

		fatalwarnings
		{
			'4263', -- member function does not override any base class virtual member function
			'4264', -- no override available for virtual member function from base 'class'; function is hidden
			'4265', -- class has virtual functions, but destructor is not virtual
			'5204', -- class has virtual functions, but its trivial destructor is not virtual; instances of objects derived from this class may not be destructed correctly
			'4555', -- result of expression not used
		}
		
		disablewarnings
		{
			'4201' -- nonstandard extension used: nameless struct/union
		}
	
	filter {}
    --[=====[ 
        # All warnings, warnings as errors, be pedantic.
        -Wall
        -Wextra
        -Werror
        -Wpedantic

        # Disable warnings about C++98 incompatibility. We're using C++20 features...
        -Wno-c++98-compat
        -Wno-c++98-compat-pedantic
        -Wno-old-style-cast
        -Wno-float-equal
        -Wno-reserved-identifier
        -Wno-newline-eof
        -Wno-switch
        -Wno-switch-enum
        -Wno-switch-default
        -Wno-covered-switch-default
        -Wno-reorder-ctor
        -Wno-ctad-maybe-unsupported
        -Wno-language-extension-token
        -Wno-global-constructors
        -Wno-missing-variable-declarations
        -Wno-exit-time-destructors
        -Wno-nonportable-system-include-path
        -Wno-sign-conversion
        -Wno-unused-member-function
        -Wno-nested-anon-types
        -Wno-gnu-anonymous-struct
        -Wno-cast-function-type
    --]=====]
end

workspace 'PhxEngine'
	configurations { 'Debug', 'Profile', 'Final' }
	platforms { clang_win_64, msvc_win_64 }
	location (workspace_directory)
	preferredtoolarchitecture('x86_64') -- Prefer this toolset on MSVC as it can handle more memory for multiprocessor compiles
	warnings('extra')
	startproject(project_name_phx_editor)
	language('C++')
	cppdialect('C++20')
	rtti('off')
	exceptionhandling ('off') -- Don't enable this setting
	objdir ("%{wks.location}/Object")
	targetdir ("%{wks.location}/Binaries/%{cfg.platform}/%{cfg.buildcfg}")

    -- For best configuration, see https://blogs.msdn.microsoft.com/vcblog/2017/07/13/precompiled-header-pch-issues-and-recommendations/
    
	flags { 'fatalcompilewarnings' }
	vectorextensions('sse4.1')
	
	filter('platforms:'..msvc_win_64)
		toolset('msc')

	filter('platforms:'..clang_win_64)
		toolset('msc-clangcl')
		--toolset('msc-llvm') -- Older versions of Clang in VS

    filter('toolset:msc*')
		flags
		{
			'multiprocessorcompile', -- /MP
		}
		
		buildoptions
		{
			'/permissive-'
		}

    filter('system:windows')
		--systemversion(os.winSdkVersion())
		entrypoint 'mainCRTStartup'
		defines
		{
			-- Define this system-wide so we have no more unexpected surprises
			'NOMINMAX', 
			'WIN32_LEAN_AND_MEAN', 
			'VC_EXTRALEAN',
			'_CRT_SECURE_NO_WARNINGS'
		}
	
    HandleGlobalWarnings()
		
	filter {}

	includedirs	{ engine_include_directory }

	filter { win_64_platform_filters }
		system('windows')
		architecture('x64')
		Platform = platform_windows
		GraphicsApis = { GraphicsApiVulkan, GraphicsApiD3D12 }
		defines
		{
			'PHX_PLATFORM_WINDOWS', 'VULKAN_API', 'D3D12_API', 'VK_USE_PLATFORM_WIN32_KHR'
		}

	--filter { 'platforms:'..VulkanOSX }
		--system('macosx')
		--architecture 'x64'
		--defines { 'VULKAN_API', 'CR_PLATFORM_MACOS', 'VK_USE_PLATFORM_MACOS_MVK' }
		
	--filter { 'platforms:'..VulkanAndroid }
		--system 'android'
		--architecture 'x64'
		--defines { 'VULKAN_API', 'CR_PLATFORM_ANDROID', 'VK_USE_PLATFORM_ANDROID_KHR' }
		
	--filter { 'platforms:'..VulkanLinux }
		--system 'linux'
		--architecture 'x64'
		--defines { 'VULKAN_API', 'CR_PLATFORM_LINUX', 'VK_USE_PLATFORM_XCB_KHR' }
		
	--filter { 'platforms:'..VulkanIOS }
		--system 'ios'
		--architecture 'x64'
		--defines { 'VULKAN_API', 'CR_PLATFORM_IOS', 'VK_USE_PLATFORM_IOS_MVK' }
		
	--filter { 'platforms:'..VulkanSwitch }
		--system 'linux'
		--architecture 'x64'
		--defines { 'VULKAN_API', 'CR_PLATFORM_SWITCH', 'VK_USE_PLATFORM_VI_NN' }
		
	filter {}
	
	-- Global library includes. Very few things should go here, basically things
	-- that are used in every possible project like math and containers
	
    
	--AddLibraryIncludes(CRSTLLibrary)
	--AddLibraryIncludes(DdsppLibrary)
	--AddLibraryIncludes(EASTLLibrary)
	--AddLibraryIncludes(HlslppLibrary)
	--AddLibraryIncludes(xxHashLibrary)

	-- Setup global include directories
	includedirs
	{
		workspace_directory,
		generated_code_directory
	}
	
	defines
	{ 
		'_HAS_EXCEPTIONS=0', -- Disable STL exceptions
	}
	
	-- Generate the global paths file
	global_variable_header = io.open(path.getabsolute(generated_code_directory)..'/GlobalVariables.h', 'wb');
	global_variable_header:write('namespace phx::GlobalPaths\n{\n');
	--global_variable_header:write('\tstatic const char* ShaderCompilerExecutableName = "'..ShaderCompilerExecutableName..'";\n');
	--global_variable_header:write('\tstatic const char* ShaderSourceDirectory = "'..path.getabsolute(SourceShadersDirectory)..'/";\n');
	--global_variable_header:write('};\n');
	global_variable_header:close();
		
	filter{}

	
	filter { 'configurations:Debug' }
		defines { 'CR_CONFIG_DEBUG' }
		optimize('off')
		--symbols('on')
		symbols('fastlink')
		--inlining('auto')

		-- We force the release runtime to be able to link against
		-- release external libraries to speed up this config
		runtime('release')

    filter { 'configurations:Profile or Final' }
		defines
		{
			'NDEBUG', -- Disables assert
			'EASTL_ASSERT_ENABLED=0'
		}
		optimize('speed')
		symbols('on')
		inlining('auto')
		flags { 'linktimeoptimization' }
		runtime('release')

    filter { 'configurations:Profile' }
		defines { 'PHX_CONFIG_PROFILING', }

	filter { 'configurations:Final' }
		defines { 'PHX_CONFIG_FINAL', }
	
    
-- Project definitions
project (project_name_phx_editor)
	kind('WindowedApp')
	files	
	{	
		editor_src_directory..'/**.h',
		editor_src_directory..'/**.cpp',
	}

	-- Libraries from other projects
	links
	{
		project_name_phx_engine,
	}
	
	-- Only executables should link to any libraries
	-- Otherwise we'll get bloated libs and slow link times
	-- Project libraries have slimmed by about ~140MB
	--LinkLibrary(library_mesh_optimizer)
	--LinkLibrary(MikkTSpaceLibrary)
	--LinkLibrary(StbLibrary)
	--LinkLibrary(WuffsLibrary)

	--AddLibraryIncludes(SDL3Library)
	--LinkLibrary(SDL3Library)
	
	--LinkLibrary(library_eastl_library)
	
	--AddLibraryIncludes(library_imgui)
	--LinkLibrary(library_imgui)

	-- todo platform filters
	--LinkLibrary(VulkanLibrary)
	--LinkLibrary(D3D12Library)
	--LinkLibrary(library_win_pix_event_runtime)

	-- Copy necessary files or DLLs
	postbuildcommands
	{
		--CopyFileCommand(path.getabsolute(SDL3Library.dlls), '%{cfg.buildtarget.directory}')
	}
	
	filter {}

--SourceShaderCompilerDirectory = SourceRenderingDirectory..'/ShaderCompiler'
--ShaderMetadataFilename = "ShaderMetadata"
--BuiltinShadersFilename = "BuiltinShaders"

project(project_name_phx_engine)
	kind('StaticLib')
	pchheader('phxengine/include/phx/phx_pch.h')
	pchsource(engine_src_directory..'/phx_pch.cpp')
	-- dependson { ProjectShaders } -- This depends on the shaders. Shaders in turn depends on the shader compiler
	-- dependson { ProjectBuiltinShaders }

	--local ShaderMetadataHeader = GeneratedShadersDirectory..'/'..ShaderMetadataFilename..'.h'
	--local ShaderMetadataCpp = GeneratedShadersDirectory..'/'..ShaderMetadataFilename..'.cpp'

	--local BuiltinShaderHeader = GeneratedShadersDirectory..'/'..BuiltinShadersFilename..'.h'
	--local BuiltinShaderCpp = GeneratedShadersDirectory..'/'..BuiltinShadersFilename..'.cpp'

	files
	{
		-- TODO handle RHI configurations
		engine_include_directory..'/**.h',
		engine_src_directory..'/**.h',
		engine_src_directory..'/**.cpp',
		
		-- The files below are autogenerated, but are manually specified so they take part in the build process
		--ShaderMetadataHeader,
		--ShaderMetadataCpp,
		--BuiltinShaderHeader,
		--BuiltinShaderCpp
	}

	ExcludePlatformSpecificRHI(engine_include_directory)
	ExcludePlatformSpecificRHI(engine_src_directory)
	
	-- TODO: DX12 Filter
	--AddLibraryIncludes(library_agility)
	--AddLibraryIncludes(library_imgui)
	
	filter { win_64_platform_filters }
		files 
		{
			engine_include_directory..'**/phx/rhi/dx12/**',
			engine_src_directory..'**/rhi/dx12/**'
		}
		--AddLibraryIncludes(VulkanLibrary)
		--AddLibraryIncludes(WinPixEventRuntimeLibrary)
		
		postbuildcommands
		{
			--CopyFileCommand(path.getabsolute(library_win_pix_event_runtime.dlls), '%{cfg.buildtarget.directory}'),
			--MakeDirCommand('%{cfg.buildtarget.directory}/D3D12/'),
			--CopyFileCommand(path.getabsolute(library_agility.dlls[1]), '%{cfg.buildtarget.directory}/D3D12/'),
			--CopyFileCommand(path.getabsolute(library_agility.dlls[2]), '%{cfg.buildtarget.directory}/D3D12/'),
		}
		
	--filter { 'platforms:'..VulkanOSX }
		--files { SourceRenderingDirectory..'/Vulkan/*' }
	
	filter {}

------------------------------------
-- Shader metadata generation job --
------------------------------------

--[=====[
local GeneratedShadersDirectoryAbsolute = path.getabsolute(GeneratedShadersDirectory)

local hlslFiles = os.matchfiles(SourceShadersDirectory..'/**.hlsl')
local shadersFiles = os.matchfiles(SourceShadersDirectory..'/**.shaders')

project(ProjectShaders)
	kind('StaticLib')
	files { SourceShadersDirectory..'/**.hlsl', SourceShadersDirectory..'/**.shaders' }
	dependson { ProjectShaderCompiler }

	local metadataFile = path.getabsolute(SourceShadersDirectory)..'/Metadata.hlsl'
	local outputFile = GeneratedShadersDirectoryAbsolute..'/'..ShaderMetadataFilename
	local shaderMetadataCommandLine = 
	'"'..ShaderCompilerPath..'" '..
	'-metadata -input "'..metadataFile..'" ' ..
	'-output "'..outputFile..'" -entrypoint metadata'

	buildcommands
	{
		'{mkdir} '..'"'..GeneratedShadersDirectoryAbsolute..'"', -- Create the output folder
		'{echo} '..shaderMetadataCommandLine, -- Echo the command line
		shaderMetadataCommandLine, -- Run
	}
	
	buildinputs { hlslFiles, shadersFiles, ShaderCompilerPath }
	
	buildoutputs { outputFile..'.uptodate' }
	
	buildmessage('')

	-- Let Visual Studio know we don't want to compile shaders through the built-in compiler
	-- In the future built in shaders could be compiled this way
	filter { 'files:**.hlsl' }
		buildaction('none')
		
	filter {}

-----------------------------------
-- Builtin shader generation job --
-----------------------------------

project(ProjectBuiltinShaders)
	kind('StaticLib')
	dependson { ProjectShaderCompiler }

	local outputFile = GeneratedShadersDirectoryAbsolute..'/BuiltinShaders'
	local builtinShaderCommandLine =
	'"'..ShaderCompilerPath..'" '..
	'-builtin -builtin-headers '..
	'-input "'..path.getabsolute(SourceShadersDirectory)..'" '..
	'-output "'..outputFile..'" '..
	'-platform '..Platform:lower()
	
	for i, graphicsApi in ipairs(GraphicsApis) do
		builtinShaderCommandLine = builtinShaderCommandLine..' -graphicsapi '..graphicsApi:lower()
	end

	buildcommands
	{
		'{echo} Compiling builtin shaders',
		'{echo} '..builtinShaderCommandLine,
		builtinShaderCommandLine,
	}

	buildinputs { hlslFiles, shadersFiles, ShaderCompilerPath }

	buildoutputs { outputFile..'.uptodate' }
	
	buildmessage('')
		
	filter {}

	--]=====]

group('.Solution Generation')

project('Generate Solution')
	kind('StaticLib')
	files{ '*.lua', '*.bat', '*.command' }

	local root_path_absolute = path.getabsolute('')
	local generate_solution_command_line = '{chdir} "'..root_path_absolute..'"'

	if is_visual_studio then
		rebuild_project_command = '"VS'..visual_studio_version..'_Dx12.bat"'
		print(rebuild_project_command)
	end

	postbuildcommands
	{
		generate_solution_command_line, -- Run
		rebuild_project_command,
	}