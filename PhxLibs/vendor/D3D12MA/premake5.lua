project "D3D12MA"
	kind "StaticLib"
	language "C++"
	cppdialect "c++17"
	
	files
	{
		'D3D12MemAlloc.cpp',
		'D3D12MemAlloc.h',
        'D3D12MemAlloc.natvis',
        'version.txt',
	}

	includedirs
	{
		"%{IncludeDir.AgilitySDK}",
	}


	filter('toolset:*-clangcl')
        buildoptions {
			'-Wno-unused-const-variable',
			'-Wno-unused-function',
		}
	filter()
	
	filter "system:windows"
		systemversion "latest"
		cppdialect "C++17"

	removefiles {}
	
	defines { 'D3D12MA_USE_AGILITY_SDK=1' }

	filter { 'configurations:*' } -- Workaround for MacOS nil in cfg

    filter {}