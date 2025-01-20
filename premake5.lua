
include "./vendor/premake/premake_customization/solution_items.lua"
include "Dependencies.lua"

workspace_directory         = '.workspace/'.._ACTION
platform_clang_win_64   = "x64 (LLVM)"
rhi_cpp_define			= ""
arg_rhi					= _ARGS[1]
executable_postfix		= ""

function ConfigureRhi()
    if arg_rhi == "d3d12" then
        rhi_cpp_define  = "PHX_RHI_D3D12"
        executable_postfix = "_d3d12"
    elseif arg_rhi == "vulkan_windows" or arg_rhi == "vulkan_linux" then
        rhi_cpp_define  = "PHX_RHI_VULKAN"
        executable_postfix = "_vulkan"
    end
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


ConfigureRhi()

-- Globals
workspace "PhxEngine"
	location (workspace_directory)
	architecture "x86_64"
	startproject "PhxEditor"
	platforms { platform_clang_win_64 }

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
    
	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")
	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
        'fatalcompilewarnings',
		"MultiProcessorCompile",
	}
    
    HandleGlobalWarnings()
    
    --filter('platforms:'..msvc_win_64)
		--toolset('msc')

    defines
    { 
        '_HAS_EXCEPTIONS=0', -- Disable STL exceptions
    }
    
	filter('platforms:'..platform_clang_win_64)
        toolset('msc-clangcl')
    --toolset('msc-llvm') -- Older versions of Clang in VS

	filter { 'configurations:Debug' }
		optimize('off')
		--symbols('on')
		symbols('fastlink')
		--inlining('auto')

		-- We force the release runtime to be able to link against
		-- release external libraries to speed up this config
		runtime('release')
        staticruntime "off"

	filter { 'configurations:Release or Dist' }
		defines
		{
			'NDEBUG', -- Disables assert
			'EASTL_ASSERT_ENABLED=0'
		}

		optimize('speed')
		symbols('on')
		inlining('auto')
		flags { 'linktimeoptimization' }
        staticruntime "off"
		runtime('release')

    filter{}

group "Dependencies"
	include "vendor/premake"
	include "Phoenix/vendor/ImGui"
	include "Phoenix/vendor/D3D12MA"
group ""

group "Core"
	include "phoenix"
group ""

group "Misc"
    include "sandbox"
group ""

group "Tools"
	--include "PhxEditor"
group ""


