-- TODO: Switch to require
include 'vendor/premake/PhxEngine/Dependencies.lua'

-- Directories
phx_lib_Directory           = 'PhxLibs'
phx_lib_src_directory       = "PhxLibs/src"
phx_lib_vendor_directory    = "PhxLibs/vendor"

phx_lib_src_core_dir        = phx_lib_src_directory.."/PhxCore"

workspace_directory         = '.workspace/'.._ACTION

-- IDE Platform Names
clang_win64_d3d12  = "Clang Win64 (D3D12)"

win64_platform_filter = "platforms:"..clang_win64_d3d12

platform_windows = "Windows"
rhi_backend_d3d12 = "D3D12"

-- Project Names
project_phx_core        = 'PhxCore'
project_phx_renderer    = 'PhxRenderer'
project_phx_rhi         = 'PhxRhi'
project_sandbox         = 'Sandbox'

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
	startproject(project_sandbox)
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
	-- include "Phoenix/vendor/ImGui"
	-- include "Phoenix/vendor/D3D12MA"
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

group ""

group "Misc"
    --include "sandbox"
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