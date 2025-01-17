project "Phoenix"
	kind "StaticLib"
	language "C++"
	cppdialect "c++20"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	pchheader "phxpch.h"
	pchsource "src/phxpch.cpp"

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
			'NOMINMAX', 
			'WIN32_LEAN_AND_MEAN', 
			'VC_EXTRALEAN',
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