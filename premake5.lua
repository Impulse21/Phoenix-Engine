--require("premake/phxengine/dependencies")
include "Dependencies.lua"

-- Globals
workspace "PhxEngine"
	location ('.workspace/'.._ACTION)
	architecture "x86_64"
	startproject "PhxEditor"
	platforms { "windows_clang" }

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
	
	configurations
	{
		"Debug",
		"Release",
		"Dist"
	}

	solution_items
	{
		".editorconfig"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

group "Dependencies"
	include "vendor/premake"
	include "Phoenix/vendor/ImGui"
	include "Phoenix/vendor/D3D12MA"
group ""

group "Core"
	include "phoenix"
group ""

group "Misc"
	--include "Sandbox"
group ""


