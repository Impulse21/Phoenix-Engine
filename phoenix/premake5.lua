project "Phoenix"
	kind "StaticLib"
	language "C++"
	cppdialect "c++20"

	
-- Note on precompiled header files
-- On Visual Studio, the header file needs to be the exact string as it appears in your include,
-- e.g. if your cpp says #include 'Foo_pch.h' then pchheader('Foo_pch.h')
-- However, the pchsource file has to be the exact path.

	pchheader "phxpch.h"
	pchsource "src/phxpch.cpp"

	files
	{
		"src/phxpch.h",
		"src/phxpch.cpp",
		"src/phx/**.h",
		"src/phx/**.cpp",
	}
	
	defines
	{
		'_CRT_SECURE_NO_WARNINGS',
	}

	includedirs
	{
		"src",
		"vendor/spdlog/include",
		"%{IncludeDir.ImGui}",
	}
	
	links
	{
	}

	filter "system:windows"
		systemversion "latest"
		defines
		{
			'PHX_PLATFORM_WINDOWS',
			'NOMINMAX', 
			'WIN32_LEAN_AND_MEAN', 
			'VC_EXTRALEAN',
		}

		files 
		{
		}

		includedirs
		{
		}

		links
		{
		}

	filter('platforms:'..platform_clang_win_64_dx12)
		defines { "PHX_RHI_D3D12" }
		
		excludes  { 'src/phx/rhi/vulkan/**' }
		files 
		{
			"src/phx/rhi/d3d12/**.h",
			"src/phx/rhi/d3d12/**.cpp",
		}

		includedirs
		{
			'src/phx/**/rhi/dx12',
			"%{IncludeDir.D3D12MA}",
			"%{IncludeDir.AgilitySDK}",
		}

	filter "configurations:Debug"
		defines { "PHX_DEBUG" }
		symbols "on"

		links
		{
		}

	filter "configurations:Release"
		defines "PHX_RELEASE"
		optimize "on"

		links
		{
		}

	filter "configurations:Dist"
		defines "PHX_DIST"
		optimize "on"

		links
		{
		}
