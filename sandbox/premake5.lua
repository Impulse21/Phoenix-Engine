
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
		"../phoenix/src",
		"../phoenix/src",
		"../phoenix/vendor",
		"%{IncludeDir.ImGui}",
		"%{IncludeDir.ENTT}",
	}

	links
	{
		"Phoenix",
		"ImGui",
	}
	
    -- Windows-specific settings
    filter "system:windows"
		defines
		{
			'PHX_PLATFORM_WINDOWS',
			'NOMINMAX', 
			'WIN32_LEAN_AND_MEAN', 
			'VC_EXTRALEAN',
		}


	filter('platforms:'..platform_clang_win_64_dx12)
		links
		{
			"D3D12MA",
			Library["D3D12"],
			Library["DXGI"],
			Library["DXGUID"],
		}

		includedirs
		{
			'../phoenix/src/phx/rhi/d3d12',
			"%{IncludeDir.AgilitySDK}",
			"%{IncludeDir.D3D12MA}",
		}

		postbuildcommands
		{
			--CopyFileCommand(path.getabsolute(WinPixEventRuntimeLibrary.dlls), '%{cfg.buildtarget.directory}'),
			MakeDirCommand('%{cfg.buildtarget.directory}D3D12/'),

			CopyFileCommand(DynamicLibrary["D3D12Core"], '%{cfg.buildtarget.directory}/D3D12/'),
			CopyFileCommand(DynamicLibrary["d3d12SDKLayers"], '%{cfg.buildtarget.directory}/D3D12/'),
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
