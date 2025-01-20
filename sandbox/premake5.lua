project "Sandbox"
	kind "WindowedApp"
	language "C++"
	cppdialect "C++20"
	
	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"../phoenix/vendor/spdlog/include",
		"../phoenix/vendor/spdlog/include",
		"../phoenix/src",
		"../phoenix/src",
		"../phoenix/vendor",
		"%{IncludeDir.ImGui}",
	}

	links
	{
		"Phoenix"
	}

	filter "system:windows"
		systemversion "latest"
		entrypoint 'wWinMain'
		
		links
		{
		}

	filter "configurations:Debug"
		defines "PHX_DEBUG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "PHX_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "PHX_DIST"
		runtime "Release"
		optimize "on"
