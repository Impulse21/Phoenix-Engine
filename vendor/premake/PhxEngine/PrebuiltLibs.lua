-- Dependencies for Corsair Engine

prebuild_direcotry      = '.workspace/PrebuiltLibs'

LibAgility              = prebuild_direcotry..'/agility_1.614.1'
LibDxc                  = prebuild_direcotry..'/dxc_2024_07_31_clang_cl'
LibDStorage             = prebuild_direcotry..'/directstorage_1.2.4'
LibPix                  = prebuild_direcotry..'/winpix_1.0.240308001'
LibDirectXTex			= prebuild_direcotry..'/directx_tex_oct2024'
LibVulkan				= prebuild_direcotry..'/vulkan_1.4.313.0'
LibSlang				= prebuild_direcotry..'/slang-2025.22.1'

AgilityLibrary =
{
	includeDirs = LibAgility..'/include',
	dlls        =
	{
		LibAgility..'/bin/x64/D3D12Core.dll',
		LibAgility..'/bin/x64/d3d12SDKLayers.dll'
	}
}

DxcLibrary =
{
	includeDirs = LibDxc..'/inc',
	libDirs     = LibDxc..'/lib/x64',
    libNames    = 'dxcompiler',
	dlls        =
	{
		LibDxc..'/bin/x64/dxcompiler.dll',
		LibDxc..'/bin/x64/dxil.dll'
	}
}

DStorageLibrary =
{
	includeDirs = LibDStorage..'/include',
	libDirs     = LibDStorage..'/lib/x64',
    libNames    = 'dstorage',
	dlls        =
	{
		LibDStorage..'/bin/x64/dstorage.dll',
		LibDStorage..'/bin/x64/dstoragecore.dll'
	}
}

SlangLibrary = 
{
	includeDirs = LibSlang..'/include',
	libDirs     = LibSlang..'/lib',
    libNames    = {'gfx', 'slang-compiler', 'slang-rt', 'slang'},
	dlls        =
	{
		LibSlang..'/bin/slang.dll',
		LibSlang..'/bin/slang-llvm.dll',
		LibSlang..'/bin/slang-rt.dll',
	}
}

PixLibrary =
{
	includeDirs = LibPix..'/include',
	libDirs     = LibPix..'/bin/x64',
    libNames    = "WinPixEventRuntime",
	dlls        =
	{
		LibPix..'/bin/x64/WinPixEventRuntime.dll',
	}
}

DirectXTexLibrary =
{
	includeDirs = LibDirectXTex..'/include',
	libDirs     = LibDirectXTex..'/lib/x64',
    libNames    = { "DirectXTex", "DirectXTex-d"}
}

VulkanLibrary =
{
	includeDirs = LibVulkan..'/Include',
	libDirs     = LibVulkan..'/Lib',
    libNames    = { "Vulkan-1"}
}

function AddLibraryIncludes(library)
	includedirs(library.includeDirs)
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
	libdirs(library.libDirs)
	links(library.libNames)
end