project "PhxEditor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++20"
	staticruntime "on"

	targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

	files
	{
		--"src/.h",
		--"src/**.cpp"
        "src/Main.cpp"
	}

	includedirs
	{
		"%{wks.location}/Hazel/vendor/spdlog/include",
		"%{wks.location}/Hazel/src",
		"%{wks.location}/Hazel/vendor",
		"%{IncludeDir.entt}",
		"%{IncludeDir.filewatch}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"Phoenix"
	}

    HandleGlobalWarnings()
    
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
