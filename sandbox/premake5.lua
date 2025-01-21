
project "Sandbox"
    kind "WindowedApp"         -- Windows application (no console)
    language "C++"
    cppdialect "C++20"         -- Use C++17

    files 
	{
        "src/**.cpp",          -- Include all .cpp files in src/
        "src/**.h",            -- Include all .h files in src/
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
		"Phoenix",
		"ImGui",
	}
	
    -- Windows-specific settings
    filter "system:windows"
        -- defines { "UNICODE", "_UNICODE" }
        -- links { "user32", "gdi32", "kernel32" } -- Link against Windows libraries

		links
		{
			"D3D12MA",
		}

    -- Debug configuration
    filter "configurations:Debug"
        defines { "PHX_DEBUG" }
        runtime "Debug"
        symbols "On"           -- Enable debug symbols

    -- Release configuration
    filter "configurations:Release"
        defines { "NDEBUG", "PHX_RELEASE" }
        runtime "Release"
        optimize "On"          -- Optimize for Release
	
	-- Release configuration
	filter "configurations:Dist"
		defines { "NDEBUG", "PHX_RELEASE" }
		runtime "Release"
		optimize "On"          -- Optimize for Release
