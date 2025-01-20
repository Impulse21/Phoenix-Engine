project "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/phoenix/vendor/spdlog/include",
		"%{wks.location}/phoenix/vendor/spdlog/include",
		"%{wks.location}/phoenix/src",
		"%{wks.location}/phoenix/vendor",
	}

	links
	{
		"Phoenix"
	}

	filter "system:windows"
		systemversion "latest"

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
