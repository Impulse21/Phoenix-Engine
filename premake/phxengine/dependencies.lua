-- Dependencies for Phx Engine

thrid_party_directory   = '3rdParty'
binary_directory = '/libraries/'
include_directory = '/include/'

-- Library Directories
lib_dir_spdlog			= thrid_party_directory..'/spdlog'
LibCGLTF              	= thrid_party_directory..'/cgltf'
LibAgility            	= thrid_party_directory..'/agility'
LibDxc                	= thrid_party_directory..'/dxc'
LibEASTL              	= thrid_party_directory..'/eastl'
LibHlslpp             	= thrid_party_directory..'/hlslpp'
LibImGui              	= thrid_party_directory..'/imgui'
LibMeshOptimizer      	= thrid_party_directory..'/meshoptimizer'
LibWinPixEventRuntime 	= thrid_party_directory..'/winpixeventruntime'
LibxxHash             	= thrid_party_directory..'/xxHash'
lib_D3D12MA				= thrid_party_directory..'/D3D12MemoryAllocator'
lib_directx_tex			= thrid_party_directory..'/DirectXTex'
lib_entt				= thrid_party_directory..'/entt'
lib_optick				= thrid_party_directory..'/optic'
lib_spdlog				= thrid_party_directory..'/spdlog'

AgilityLibrary =
{
	include_dirs = LibAgility..include_directory..'include',
	lib_dirs     = LibAgility..binary_directory,
	dlls        =
	{
		LibAgility..binary_directory..'x64/D3D12Core.dll',
		LibAgility..binary_directory..'x64/d3d12SDKLayers.dll'
	}
}

-- natvis      = LibCRSTL..include_directory..'include/*.natvis',

CGLTFLibrary =
{
	include_dirs = LibCGLTF..include_directory
}

SpdLogLibrary = 
{
	include_dirs = lib_dir_spdlog..include_directory
}

D3D12Library =
{
	lib_names = {'dxgi', 'd3d12'}
}

-- https://github.com/microsoft/DirectXShaderCompiler
DxcLibrary =
{
	include_dirs = LibDxc..include_directory,
	lib_dirs     = LibDxc..binary_directory,
	lib_names    = { 'dxcompiler' },
	dlls        = { LibDxc..binary_directory..'dxcompiler.dll', LibDxc..binary_directory..'dxil.dll' }
}

EASTLLibrary =
{
	include_dirs =
	{ 
		LibEASTL..include_directory..'include',
		LibEASTL..include_directory..'test/packages/EAStdC/include', 
		LibEASTL..include_directory..'test/packages/EAAssert/include',
		LibEASTL..include_directory..'test/packages/EABase/include/Common'
	},
	defines =
	{
		"EASTL_ASSERT_ENABLED=1",
		"CHAR8_T_DEFINED" -- We don't want EASTL to define char8_t regardless of compiler options
	},
	natvis      = LibEASTL..include_directory..'doc/**.natvis',
	lib_dirs     = LibEASTL..binary_directory,
	lib_names    = 'EASTL.vs2019.release'
}

HlslppLibrary =
{
	include_dirs = LibHlslpp..include_directory..'include',
	files = LibHlslpp..include_directory..'include/**.h',
	natvis = LibHlslpp..include_directory..'include/**.natvis',
	defines = 'HLSLPP_FEATURE_TRANSFORM'
}

ImguiLibrary =
{
	include_dirs = LibImGui..include_directory,
	lib_dirs     = LibImGui..binary_directory,
	lib_names    = 'ImGui.vs2022.release',
	defines = 'IMGUI_DISABLE_OBSOLETE_FUNCTIONS'
}

MeshOptimizerLibrary =
{
	include_dirs = LibMeshOptimizer..include_directory..'src',
	lib_dirs     = LibMeshOptimizer..binary_directory,
	lib_names    = 'MeshOptimizer.vs2022.release',
}

WinPixEventRuntimeLibrary =
{
	include_dirs = { LibWinPixEventRuntime..include_directory },
	lib_dirs = { LibWinPixEventRuntime..binary_directory },
	lib_names = 'WinPixEventRuntime',
	dlls = LibWinPixEventRuntime..binary_directory..'WinPixEventRuntime.dll'
}

xxHashLibrary =
{
	include_dirs = LibxxHash..include_directory
}

function AddLibraryIncludes(library)
	include_dirs(library.include_dirs)
	if(library['defines']) then
		defines(library.defines)
	end
end

-- Add files to the solution. This should generally not include cpp files
-- and most of the time is to aid intellisense and parsing of includes
function AddLibraryFiles(library)
	files(library.files)
end

function AddLibraryNatvis(library)
	files(library.natvis)
end

function LinkLibrary(library)
	lib_dirs(library.lib_dirs)
	links(library.lib_names)
end