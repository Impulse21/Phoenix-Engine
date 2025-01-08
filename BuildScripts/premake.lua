--require("premake/phxengine/dependencies")

-- Globals

workspace_directory	= '../.workspace/'.._ACTION

generated_shader_directory  = workspace_directory..'/GeneratedShaders'
generated_code_directory    = workspace_directory..'/GeneratedCode'

cpp_version 			= 'c++20'
sln_name				= 'PhxEngine'
editor_project_name		= 'PhxEditor'
engine_project_name		= 'PhxEngine'
editor_directory		= 'phxeditor'
engine_directory		= 'phxengine'
third_party_directory	= '3rdparty'

executable_name 		= editor_project_name

engine_include_directory 	= engine_directory..'/include'        
engine_src_directory 		= engine_directory..'/src'

rhi_cpp_define			= ""
arg_rhi					= _ARGS[1]

rhi_includes = 
{
	vulkan_windows = 
	{

	}
}

rhi_excludes =
{
	d3d12 = 
	{ 
		engine_include_directory..'phx/rhi/dx12/**',
		engine_src_directory..'rhi/dx12/**',
	},
	vulkan =
	{
		engine_include_directory..'phx/rhi/vulkan/**',
		engine_src_directory..'rhi/vulkan/**',
	}

}


rhi_libraries =
{
	d3d12 = 
	{
		release = 
		{
			-- No specific D3D12 release libraries
		},
		debug = 
		{
			-- No specific D3D12 debug libraries
		}
	},
	vulkan_windows = 
	{
		release = 
		{
		},
		debug = 
		{
		}
	},
	vulkan_linux = 
	{
		release = 
		{
		},
		debug = 
		{
		}
	}
}

include_dir = {}
include_dir["agility"] = "%{wks.location}/../PrebuiltLibs/agility_1.614.1/include"

function ConfigureRhi()
    if arg_rhi == "d3d12" then
        rhi_cpp_define  = "PHX_RHI_D3D12"
        executable_name = executable_name .. "_d3d12"
    elseif arg_rhi == "vulkan_windows" or arg_rhi == "vulkan_linux" then
        rhi_cpp_define  = "PHX_RHI_VULKAN"
        executable_name = executable_name .. "_vulkan"
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
            "-Wno-cast-function-type"         -- Disable warnings about function pointer casts
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


function SlnConfiguration()
	
	workspace(sln_name)
		configurations { 'debug', 'final' }
		language('C++')
		warnings('extra')
		startproject(editor_project_name)
		cppdialect(cpp_version)
		rtti('off')
		exceptionhandling ('on') -- Don't enable this setting
		objdir ("%{wks.location}/.build/object")
		targetdir ("%{wks.location}/.build/binaries/%{cfg.platform}/%{cfg.buildcfg}")
		
		-- platforms
		if os.target() == "windows" then
			platforms { "windows_clang" }
		elseif os.target() == "linux" then
			platforms { "linux" }
		end

		location (workspace_directory)
		
        -- system & architecture
        if os.target() == "windows" then
            filter { "platforms:windows_*" }
                system "windows"
                architecture "x64"
				buildoptions { "/arch:AVX2" }
		elseif os.target() == "linux" then
            filter { "platforms:linux" }
                system "linux"
                architecture "x86_64"
				buildoptions { "-mavx2" }
		end
	
		
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
	
    -- "Debug"
    filter "configurations:debug"
    	defines { "DEBUG" }
		flags { "MultiProcessorCompile" }
		optimize "Off"
		symbols "On"
		debugformat "c7"

	-- "Release"
	filter "configurations:final"
    	defines { 'NDEBUG', 'PHX_CONFIG_FINAL' }
		flags { "MultiProcessorCompile", "LinkTimeOptimization" }
		optimize "Speed"
		symbols "Off"
		inlining('auto')
		runtime('release')

	filter{}
end

function SlnGenerationConfiguration()
	group('.Solution Generation')
	project('Generate Solution')
		kind('StaticLib')
		files{ '../**.lua', '../**.bat', '../**.command', '../**.py' }
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
	group()
end

function ThridPartyConfgurations()
	group "3rd Party"
	include('../3rdParty/Imgui/premake.lua')
	include('../3rdParty/D3D12MA/premake.lua')
	group ""
end

ConfigureRhi()
SlnConfiguration()
HandleGlobalWarnings()

ThridPartyConfgurations()

SlnGenerationConfiguration()