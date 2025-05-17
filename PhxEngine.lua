include "vendor/premake/PhxEngine/PrebuiltLibs.lua"

-- Directories
phx_lib_Directory           = 'PhxLibs'
phx_lib_src_directory       = "PhxLibs/src"
phx_lib_vendor_directory    = "PhxLibs/vendor"
phx_app_directory           = 'PhxApps'

phx_lib_src_core_dir        = phx_lib_src_directory.."/PhxCore"
phx_lib_src_rhi_dir         = phx_lib_src_directory.."/PhxRhi"
phx_lib_src_renderer_dir    = phx_lib_src_directory.."/PhxRenderer"
phx_lib_src_resource_dir    = phx_lib_src_directory.."/PhxResource"
phx_lib_src_world_dir       = phx_lib_src_directory.."/PhxWorld"
phx_lib_src_data_dir        = phx_lib_src_directory.."/PhxData"
phx_lib_src_engine_dir      = phx_lib_src_directory.."/PhxEngine"

phx_vendor_src_imgui_dir    = phx_lib_vendor_directory.."/ImGui"
phx_vendor_src_d3d12ma_dir  = phx_lib_vendor_directory.."/D3D12MA"
phx_vendor_src_entt_dir     = phx_lib_vendor_directory.."/entt"
phx_vendor_src_yaml_dir     = phx_lib_vendor_directory.."/yaml"
phx_vendor_src_glfw_dir     = phx_lib_vendor_directory.."/glfw"
phx_vendor_src_json_dir     = phx_lib_vendor_directory.."/json"
phx_vendor_src_cereal_dir   = phx_lib_vendor_directory.."/cereal"

phx_vendor_include_glfw_dir = phx_vendor_src_glfw_dir.."/include"
phx_vendor_include_yaml_dir = phx_vendor_src_yaml_dir.."/include"

phx_packer_vendor_dir       = "PhxAssetPacker/vendor"
phx_packer_vendor_dx_tex    = phx_packer_vendor_dir..'/DirectXTex'

phx_script_dir                  = "scripts"
phx_generate_reflection_script  = phx_script_dir.."/generate_reflection.py"
phx_generated_file_name         = "GeneratedReflection.gen.cpp"
phx_reflection_output_dir       = phx_lib_src_data_dir.."/"..phx_generated_file_name
workspace_directory             = '.workspace/'.._ACTION

-- IDE Platform Names
clang_win64_d3d12  = "Clang Win64 (D3D12)"

-- TOODL Add vulkan to this list = "platforms:"..clang_win64_d3d12..' or '..clang_win64_vulkan
win64_platform_filter = "platforms:"..clang_win64_d3d12

--Win64PlatformFilter 
platform_windows = "Windows"
rhi_backend_d3d12 = "D3D12"

-- Project Names
project_phx_core        = 'PhxCore'
project_phx_renderer    = 'PhxRenderer'
project_phx_rhi         = 'PhxRhi'
project_phx_resource    = 'PhxResource'
project_phx_world       = 'PhxWorld'
project_phx_data        = 'PhxData'
project_phx_engine      = 'PhxEngine'

project_phx_app_editor  = 'PhxEditor'
project_sandbox         = 'Sandbox'
project_asset_packer    = 'PhxAssetPacker'

project_vendor_imgui    = 'ImGui'
project_vendor_d3d12ma  = 'D3D12MA'
project_vendor_yaml     = 'yaml-cpp'

-- Generated Code Directories
generated_shaders_dir = workspace_directory..'/GeneratedShaders'
generated_code_dir = workspace_directory..'/GeneratedCode'

assets_directory = workspace_directory.."/../assets"

-- Utility Functions
function ExcludePlatformSpecificCode(rootPath)
	excludes { rootPath..'**/platform/**' }
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
			'4201', -- nonstandard extension used: nameless struct/union
			'5038', -- -Wno-reorder-ctor
		}
		
	
	filter('toolset:*-clangcl')
        buildoptions {
            -- Compatibility warnings
            "-Wno-c++98-compat",              -- Disable C++98 compatibility warnings
            "-Wno-c++98-compat-pedantic",     -- Disable pedantic C++98 compatibility warnings

            -- Coding style warnings
            "-Wno-old-style-cast",            -- Disable warnings about old-style C-style casts
            "-Wno-float-equal",               -- Disable warnings about floating-point equality checks
            "-Wno-reserved-identifier",       -- Disable warnings about reserved identifiers
            "-Wno-newline-eof",               -- Disable warnings about missing newline at end of file
            "-Wno-switch",                    -- Disable general switch statement warnings
            "-Wno-switch-enum",               -- Disable warnings about missing cases in switch for enums
            "-Wno-switch-default",            -- Disable warnings about missing default in switch statements
            "-Wno-reorder-ctor",              -- Disable warnings about constructor initializer order
            "-Wno-covered-switch-default",    -- Disable warnings about covered default cases in switch
            "-Wno-ctad-maybe-unsupported",    -- Disable warnings about potentially unsupported CTAD usage

            -- Language extensions
            "-Wno-language-extension-token",  -- Disable warnings about use of language extension tokens
            "-Wno-global-constructors",       -- Disable warnings about global constructors
            "-Wno-missing-variable-declarations", -- Disable warnings about missing variable declarations
            "-Wno-exit-time-destructors",     -- Disable warnings about destructors called at exit time
            "-Wno-nonportable-system-include-path", -- Disable warnings about non-portable include paths

            -- Conversions and usage warnings
            "-Wno-sign-conversion",           -- Disable warnings about signed/unsigned conversions
            "-Wno-unused-member-function",    -- Disable warnings about unused member functions

            -- Anonymous structures and casting
            "-Wno-nested-anon-types",         -- Disable warnings about nested anonymous types
            "-Wno-gnu-anonymous-struct",      -- Disable warnings about GNU anonymous structs
            "-Wno-cast-function-type",        -- Disable warnings about function pointer casts

            "-Wno-misleading-indentation",
            "-Wno-tautological-undefined-compare",

            -- Cereal issues
            "-Wno-deprecated-declarations",
        }
		
		
	filter { "files:3rdParty/**" }
    	warnings "Off"

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
        -Wno-reorder-ctor
        -Wno-covered-switch-default
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

-- Globals
workspace "PhxEngine"
    configurations { 'Debug', 'Profiling', 'Final' }
	location (workspace_directory)
    preferredtoolarchitecture('x86_64') -- Prefer this toolset on MSVC as it can handle more memory for multiprocessor compiles
    warnings('extra')
	startproject(project_phx_app_editor)
    language('C++')
	cppdialect('C++20')
	rtti('off')
	platforms { clang_win64_d3d12 }

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	flags { 'fatalcompilewarnings' }

	filter('platforms:'..clang_win64_d3d12)
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
		--entrypoint 'mainCRTStartup'
		defines
		{
			-- Define this system-wide so we have no more unexpected surprises
			'NOMINMAX', 
			'WIN32_LEAN_AND_MEAN', 
			'VC_EXTRALEAN',
			'_CRT_SECURE_NO_WARNINGS'
		}		
	filter {}
    
    HandleGlobalWarnings()

    -- Setup global include directories
	includedirs
	{
		generated_code_dir
	}

	-- Generate the global paths file
	globalVariableHeader = io.open(path.getabsolute(generated_code_dir)..'/Generated/GlobalVariables.h', 'wb');
	globalVariableHeader:write('namespace phx::GlobalPaths\n{\n');
	globalVariableHeader:write('\tconstexpr const char* WorkspaceDirectory = "'..path.getabsolute(workspace_directory)..'/";\n');
	globalVariableHeader:write('\tconstexpr const char* AssetsDirectory = "'..path.getabsolute(assets_directory)..'/";\n');
	--globalVariableHeader:write('\tstatic const char* ShaderCompilerExecutableName = "'..ShaderCompilerExecutableName..'";\n');
	--globalVariableHeader:write('\tstatic const char* ShaderSourceDirectory = "'..path.getabsolute(SourceShadersDirectory)..'/";\n');
	globalVariableHeader:write('};\n');
	globalVariableHeader:close();

	filter {}
		
	filter { win64_platform_filter }
		system('windows')
		architecture('x64')
		defines
		{
			'PHX_PLATFORM_WINDOWS'
		}

    filter {}

	filter { 'configurations:Debug' }
		defines { 'PHX_DEBUG' }
		optimize('off')
		--symbols('on')
		symbols('fastlink')
		--inlining('auto')

    filter { 'configurations:Profiling or Final' }
		defines
		{
			'NDEBUG', -- Disables assert
			'EASTL_ASSERT_ENABLED=0'
		}
		optimize('speed')
		symbols('on')
		inlining('auto')
		flags { 'linktimeoptimization' }

    filter { 'configurations:Profiling' }
		defines { 'PHX_PROFILING', }

	filter { 'configurations:Final' }
		defines { 'PHX_FINAL', }

-- Project definitions

group "Vendors"
    project(project_vendor_imgui)
        kind "StaticLib"
        language "C++"
        cppdialect "c++17"

        files
        {
            phx_vendor_src_imgui_dir..'/*.cpp',
            phx_vendor_src_imgui_dir..'/*.hpp',
            phx_vendor_src_imgui_dir..'/*.h',
            phx_vendor_src_imgui_dir..'/version.txt',
            phx_vendor_src_imgui_dir..'/LICENSE.txt'
        }

        filter "system:linux"
            pic "On"
            systemversion "latest"
        removefiles {}

        defines { 'IMGUI_DISABLE_OBSOLETE_FUNCTIONS' }

        filter { 'configurations:*' } -- Workaround for MacOS nil in cfg

        filter {}

    project(project_vendor_d3d12ma)
        kind "StaticLib"
        language "C++"
        
        files
        {
            phx_vendor_src_d3d12ma_dir..'/D3D12MemAlloc.cpp',
            phx_vendor_src_d3d12ma_dir..'/D3D12MemAlloc.h',
            phx_vendor_src_d3d12ma_dir..'/D3D12MemAlloc.natvis',
            phx_vendor_src_d3d12ma_dir..'/version.txt',
        }

        AddLibraryIncludes(AgilityLibrary)

        filter('toolset:*-clangcl')
            buildoptions {
                '-Wno-unused-const-variable',
                '-Wno-unused-function',
                '-Wno-unused-parameter',
                '-Wno-missing-field-initializers',
            }
        filter()
        
        filter "system:windows"
            systemversion "latest"
            cppdialect "C++17"

        removefiles {}
        
        defines { 'D3D12MA_USE_AGILITY_SDK=1' }

        filter { 'configurations:*' } -- Workaround for MacOS nil in cfg

        filter {}

    project(project_vendor_yaml)
        kind "StaticLib"
        files
        {
            phx_vendor_src_yaml_dir.."/src/**.h",
            phx_vendor_src_yaml_dir.."/src/**.cpp",
            
            phx_vendor_src_yaml_dir.."/include/**.h"
        }
    
        includedirs
        {
            phx_vendor_src_yaml_dir.."/include"
        }
        
        defines { "YAML_CPP_STATIC_DEFINE" }
        filter { 'configurations:*' } -- Workaround for MacOS nil in cfg

        filter {}
        
    project "GLFW"
        kind "StaticLib"
        language "C"

        files
        {
            phx_vendor_include_glfw_dir.."/GLFW/glfw3.h",
            phx_vendor_include_glfw_dir.."/GLFW/glfw3native.h",
            phx_vendor_src_glfw_dir.."/src/glfw_config.h",
            phx_vendor_src_glfw_dir.."/src/context.c",
            phx_vendor_src_glfw_dir.."/src/init.c",
            phx_vendor_src_glfw_dir.."/src/input.c",
            phx_vendor_src_glfw_dir.."/src/monitor.c",
            phx_vendor_src_glfw_dir.."/src/vulkan.c",
            phx_vendor_src_glfw_dir.."/src/window.c"
        }
        filter "system:linux"
            pic "On"

            systemversion "latest"
            staticruntime "On"

            files
            {
                phx_vendor_src_glfw_dir.."/src/x11_init.c",
                phx_vendor_src_glfw_dir.."/src/x11_monitor.c",
                phx_vendor_src_glfw_dir.."/src/x11_window.c",
                phx_vendor_src_glfw_dir.."/src/xkb_unicode.c",
                phx_vendor_src_glfw_dir.."/src/posix_time.c",
                phx_vendor_src_glfw_dir.."/src/posix_thread.c",
                phx_vendor_src_glfw_dir.."/src/glx_context.c",
                phx_vendor_src_glfw_dir.."/src/egl_context.c",
                phx_vendor_src_glfw_dir.."/src/osmesa_context.c",
                phx_vendor_src_glfw_dir.."/src/linux_joystick.c"
            }

            defines
            {
                "_GLFW_X11"
            }

        filter "system:windows"
            systemversion "latest"
            staticruntime "On"

            files
            {
                phx_vendor_src_glfw_dir.."/src/win32_init.c",
                phx_vendor_src_glfw_dir.."/src/win32_joystick.c",
                phx_vendor_src_glfw_dir.."/src/win32_monitor.c",
                phx_vendor_src_glfw_dir.."/src/win32_time.c",
                phx_vendor_src_glfw_dir.."/src/win32_thread.c",
                phx_vendor_src_glfw_dir.."/src/win32_window.c",
                phx_vendor_src_glfw_dir.."/src/wgl_context.c",
                phx_vendor_src_glfw_dir.."/src/egl_context.c",
                phx_vendor_src_glfw_dir.."/src/osmesa_context.c"
            }

            defines 
            { 
                "_GLFW_WIN32",
                "_CRT_SECURE_NO_WARNINGS"
            }

        filter('toolset:*-clangcl')
            buildoptions {
                '-Wno-unused-const-variable',
                '-Wno-unused-function',
                '-Wno-unused-parameter',
                '-Wno-missing-field-initializers',
                '-Wno-sign-compare',
            }

        filter { 'configurations:*' } -- Workaround for MacOS nil in cfg
            
        filter {}
group ""

group "PhxLibs"
    project(project_phx_core)
        kind('StaticLib')
        pchheader('PhxCore/PhxCore_pch.h')
        pchsource(phx_lib_src_core_dir..'/PhxCore_pch.cpp')
        
        files
        {
            phx_lib_src_core_dir.."/**.h",
            phx_lib_src_core_dir.."/**.cpp",
        }

        includedirs
        {
            phx_lib_src_directory,
            phx_lib_vendor_directory.."/spdlog/include",
        }
        
        filter('platforms:'..clang_win64_d3d12)
            AddLibraryIncludes(DStorageLibrary)
        filter{}

    project(project_phx_rhi)
        kind('StaticLib')
        pchheader('PhxRhi/PhxRhi_pch.h')
        pchsource(phx_lib_src_rhi_dir..'/PhxRhi_pch.cpp')
        
		files 
		{
			phx_lib_src_rhi_dir.."/**.h",
			phx_lib_src_rhi_dir.."/**.cpp",
		}

        includedirs
        {
            phx_lib_src_directory,
            phx_lib_vendor_directory.."/spdlog/include",
        }

        filter('platforms:'..clang_win64_d3d12)
            defines { "PHX_RHI_D3D12" }
            
            excludes  { phx_lib_src_rhi_dir..'/vulkan/**' }

            files 
            {
                phx_lib_src_rhi_dir.."/d3d12/**.h",
                phx_lib_src_rhi_dir.."/d3d12/**.cpp",
            }
            
            AddLibraryIncludes(AgilityLibrary)

            includedirs
            {
                phx_lib_src_rhi_dir..'/d3d12',
                phx_vendor_src_d3d12ma_dir,
            }
        filter {}
        
    project(project_phx_renderer)
        kind('StaticLib')
        pchheader('PhxRenderer/PhxRenderer_pch.h')
        pchsource(phx_lib_src_renderer_dir..'/PhxRenderer_pch.cpp')
        
        files 
        {
            phx_lib_src_renderer_dir.."/**.h",
            phx_lib_src_renderer_dir.."/**.cpp",
        }
    
        includedirs
        {
            phx_lib_src_directory,
            phx_vendor_src_imgui_dir,
            phx_lib_vendor_directory.."/spdlog/include",
            phx_lib_vendor_directory.."/entt",
            phx_vendor_src_cereal_dir,
        }

        filter('platforms:'..clang_win64_d3d12)
            defines { "PHX_RHI_D3D12" }
            
            excludes  { phx_lib_src_rhi_dir..'/vulkan/**' }
    
            AddLibraryIncludes(AgilityLibrary)
    
            includedirs
            {
                phx_lib_src_rhi_dir..'/d3d12',
                phx_vendor_src_d3d12ma_dir,
            }

        filter{}

    project(project_phx_engine)
        kind('StaticLib')
        pchheader('PhxEngine/PhxEngine_pch.h')
        pchsource(phx_lib_src_engine_dir..'/PhxEngine_pch.cpp')
        
        files 
        {
            phx_lib_src_engine_dir.."/**.h",
            phx_lib_src_engine_dir.."/**.cpp",
        }
    
        includedirs
        {
            phx_lib_src_directory,
            phx_vendor_src_imgui_dir,
            phx_lib_vendor_directory.."/spdlog/include",
        }

        filter('platforms:'..clang_win64_d3d12)
            defines { "PHX_RHI_D3D12" }
            
            excludes  { phx_lib_src_rhi_dir..'/vulkan/**' }
    
            AddLibraryIncludes(AgilityLibrary)
    
            includedirs
            {
                phx_lib_src_rhi_dir..'/d3d12',
                phx_vendor_src_d3d12ma_dir,
            }

        filter{}

    project(project_phx_resource)
        kind('StaticLib')
        pchheader('PhxResource/PhxResource_pch.h')
        pchsource(phx_lib_src_resource_dir..'/PhxResource_pch.cpp')
            
        files 
        {
            phx_lib_src_resource_dir.."/**.h",
            phx_lib_src_resource_dir.."/**.cpp",
        }
        
        includedirs
        {
            phx_lib_src_directory,
            phx_vendor_src_imgui_dir,
            phx_lib_vendor_directory.."/spdlog/include",
        }
        
        -- TODO: Do a better job at abtracting this away.
        filter('platforms:'..clang_win64_d3d12)
            defines { "PHX_RHI_D3D12" }
            AddLibraryIncludes(DStorageLibrary)
            AddLibraryIncludes(AgilityLibrary)
    
            includedirs
            {
                phx_lib_src_rhi_dir..'/d3d12',
                phx_vendor_src_d3d12ma_dir,
            }
    
        filter{}
        
    project(project_phx_data)
        kind('StaticLib')
        pchheader('PhxData/PhxData_pch.h')
        pchsource(phx_lib_src_data_dir..'/PhxData_pch.cpp')
        
        files
        {
            phx_lib_src_data_dir.."/**.h",
            phx_lib_src_data_dir.."/**.cpp",
            --phx_lib_src_data_dir.."/**.py",
            --phx_lib_src_data_dir.."/"..phx_generated_file_name,
            --phx_generate_reflection_script,
        }

        includedirs
        {
            phx_lib_src_directory,
            phx_lib_vendor_directory.."/spdlog/include",
            phx_lib_vendor_directory.."/entt",
            phx_vendor_include_yaml_dir,
            phx_vendor_src_cereal_dir,
        }

        defines { "YAML_CPP_STATIC_DEFINE" }

        -- Pre-build step to generate reflection
        --[[prebuildcommands {
            "python ../../"..phx_generate_reflection_script.." --output ../../"..phx_reflection_output_dir.." ../../"..phx_lib_src_data_dir
        }--]]

    project(project_phx_world)
        kind('StaticLib')
        pchheader('PhxWorld/PhxWorld_pch.h')
        pchsource(phx_lib_src_world_dir..'/PhxWorld_pch.cpp')
        
        files
        {
            phx_lib_src_world_dir.."/**.h",
            phx_lib_src_world_dir.."/**.cpp",
        }

        removefiles {
            phx_lib_src_world_dir.."/vendor/meshoptimizer/Demo/*.h",
            phx_lib_src_world_dir.."/vendor/meshoptimizer/Demo/*.cpp",
        }

        includedirs
        {
            phx_lib_src_directory,
            phx_lib_vendor_directory.."/spdlog/include",
            phx_lib_vendor_directory.."/entt",
            phx_vendor_include_yaml_dir,
            phx_vendor_src_json_dir,
            phx_vendor_src_cereal_dir,
        }

        defines { "YAML_CPP_STATIC_DEFINE" }
      
group ""

group "Applications"
    project(project_phx_app_editor)
        kind "WindowedApp"         -- Windows application (no console)

        files 
        {
            phx_app_directory.."/"..project_phx_app_editor.."/src/**.cpp",
            phx_app_directory.."/"..project_phx_app_editor.."/src/**.h",

            -- Vendor stuff
            phx_app_directory.."/"..project_phx_app_editor.."/vendor/tinyobj/**.cc",
            phx_app_directory.."/"..project_phx_app_editor.."/vendor/tinyobj/**.h",
            phx_app_directory.."/"..project_phx_app_editor.."/vendor/fast_obj/**.c",
            phx_app_directory.."/"..project_phx_app_editor.."/vendor/fast_obj/**.h",
            phx_app_directory.."/"..project_phx_app_editor.."/vendor/meshoptimizer/**.cpp",
            phx_app_directory.."/"..project_phx_app_editor.."/vendor/meshoptimizer/**.h"
        }

        includedirs 
        {
            phx_lib_src_directory,
            phx_lib_vendor_directory.."/spdlog/include",
            phx_lib_vendor_directory.."/cgltf",
            phx_lib_vendor_directory.."/entt",
            phx_vendor_src_imgui_dir,
            phx_vendor_src_entt_dir,
            phx_vendor_src_cereal_dir,
            phx_app_directory.."/"..project_phx_app_editor.."/vendor"
        }

        links
        {
            project_phx_core,
            project_phx_rhi,
            project_phx_renderer,
            project_phx_resource,
            project_phx_world,
            project_phx_data,
            project_phx_engine,
            project_vendor_imgui,
        }
        
        filter('platforms:'..clang_win64_d3d12)
            defines { "PHX_RHI_D3D12" }

            AddLibraryIncludes(AgilityLibrary)
            AddLibraryIncludes(DStorageLibrary)

            LinkLibrary(DStorageLibrary)
            LinkLibrary(DxcLibrary)
            LinkLibrary(PixLibrary)
            
            links
            {
                project_vendor_d3d12ma,
                "d3d12.lib",
                "dxgi.lib",
                "dxguid.lib",
            }

            includedirs
            {
                phx_lib_src_rhi_dir..'/d3d12',
                phx_vendor_src_d3d12ma_dir,
            }

            postbuildcommands
            {
		        CopyFileCommand(path.getabsolute(DStorageLibrary.dlls[1]), '%{cfg.buildtarget.directory}'),
		        CopyFileCommand(path.getabsolute(DStorageLibrary.dlls[2]), '%{cfg.buildtarget.directory}'),

		        CopyFileCommand(path.getabsolute(DxcLibrary.dlls[1]), '%{cfg.buildtarget.directory}'),
		        CopyFileCommand(path.getabsolute(DxcLibrary.dlls[2]), '%{cfg.buildtarget.directory}'),

		        CopyFileCommand(path.getabsolute(PixLibrary.dlls[1]), '%{cfg.buildtarget.directory}'),

                MakeDirCommand('%{cfg.buildtarget.directory}/D3D12/'),
                CopyFileCommand(path.getabsolute(AgilityLibrary.dlls[1]), '%{cfg.buildtarget.directory}/D3D12/'),
                CopyFileCommand(path.getabsolute(AgilityLibrary.dlls[2]), '%{cfg.buildtarget.directory}/D3D12/'),
            }
            
        filter{}
    project(project_asset_packer)
        kind "ConsoleApp"         -- Windows application (no console)
        
        defines { "YAML_CPP_STATIC_DEFINE" }

        files 
        {
            "PhxAssetPacker/src/**.cpp",          -- Include all .cpp files in src/
            "PhxAssetPacker/src/**.h",            -- Include all .h files in src/
        }

        AddLibraryIncludes(DStorageLibrary)
        LinkLibrary(DStorageLibrary)
        
        includedirs 
        {
            phx_lib_src_directory,
            phx_lib_vendor_directory.."/spdlog/include",
            phx_lib_vendor_directory.."/cgltf",
            phx_lib_vendor_directory.."/entt",
            phx_vendor_include_yaml_dir,
            phx_vendor_src_json_dir,
            phx_vendor_src_cereal_dir,
        }

        links
        {
            project_phx_core,
            project_phx_rhi,
            project_phx_renderer,
            project_phx_resource,
            project_phx_world,
            project_phx_engine,
            project_phx_data,
            project_vendor_imgui,
            project_vendor_yaml,
        }
        
        postbuildcommands
        {
            CopyFileCommand(path.getabsolute(DStorageLibrary.dlls[1]), '%{cfg.buildtarget.directory}'),
            CopyFileCommand(path.getabsolute(DStorageLibrary.dlls[2]), '%{cfg.buildtarget.directory}'),
        }
        
        -- TODO: Do a better job at abtracting this away.
        filter('platforms:'..clang_win64_d3d12)
            defines { "PHX_RHI_D3D12" }
            AddLibraryIncludes(AgilityLibrary)
    
            includedirs
            {
                phx_lib_src_rhi_dir..'/d3d12',
                phx_vendor_src_d3d12ma_dir,
            }
    
            AddLibraryIncludes(DirectXTexLibrary)
            libdirs(DirectXTexLibrary.libDirs)
            filter { 'configurations:Debug' }
                links(DirectXTexLibrary.libNames[2])
    
            filter { 'configurations:Profiling or Final' }
                links(DirectXTexLibrary.libNames[1])
        filter{}

group ""

group '.Solution Generation'
    project('Generate Solution')
        kind('StaticLib')
        files{ '*.lua', '*.bat', '*.command' }

        local rootPathAbsolute = path.getabsolute('')
        local generateSolutionCommandLine = '{chdir} "'..rootPathAbsolute..'"'

        if IsVisualStudio then
            rebuildProjectCommand = '"Visual Studio '..VisualStudioVersion..'.bat"'
            print(rebuildProjectCommand)
        end

        postbuildcommands
        {
            generateSolutionCommandLine, -- Run
            rebuildProjectCommand,
        }
        
group ""

--[[
group '.Scripts'
    project('Scripts')
        kind('StaticLib')
        files{ phx_generate_reflection_script }
group ""
--]]