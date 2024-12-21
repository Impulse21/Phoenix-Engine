require("premake/phxengine/dependencies")

-- Directories
engine_directory            = 'phxengine'
engine_include_directory    = engine_directory..'/include'        
engine_src_directory        = engine_directory..'/src'

editor_directory            = 'phxeditor'
editor_include_directory    = editor_directory..'/include'
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