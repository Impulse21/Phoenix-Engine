project "Phoenix"
	kind "StaticLib"
	language "C++"
	cppdialect "c++20"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "phxpch.h"
	pchsource "src/phxpch.cpp"

	
	excludes { 'src/phx/**/rhi/d3d12/**' }
	excludes { 'src/phx/**/rhi/vulkan/**' }

	HandleGlobalWarnings()
	
	files
	{
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
		"ImGui",
	}

	filter "system:windows"
		systemversion "latest"
		defines
		{
			'PHX_PLATFORM_WINDOWS',
			'NOMINMAX', 
			'WIN32_LEAN_AND_MEAN', 
			'VC_EXTRALEAN',
			"%{rhi_cpp_define}",
		}

		includedirs	{ 'src/phx/**/rhi/dx12' }

		files 
		{
			"src/phx/rhi/d3d12/**.h",
			"src/phx/rhi/d3d12/**.cpp",
		}

		includedirs
		{
			"%{IncludeDir.D3D12MA}",
			"%{IncludeDir.AgilitySDK}",
		}

		links
		{
			"D3D12MA",
		}

	filter "configurations:Debug"
		defines "PHX_DEBUG"
		runtime "Debug"
		symbols "on"

		links
		{
		}

	filter "configurations:Release"
		defines "PHX_RELEASE"
		runtime "Release"
		optimize "on"

		links
		{
		}

	filter "configurations:Dist"
		defines "PHX_DIST"
		runtime "Release"
		optimize "on"

		links
		{
		}
