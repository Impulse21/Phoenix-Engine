--require("premake/phxengine/dependencies")
include "Dependencies.lua"

-- Globals
workspace "PhxEngine"
	location ('.workspace/'.._ACTION)
	architecture "x86_64"
	startproject "PhxEditor"
	platforms { "windows_clang" }
	objdir ("%{wks.location}/.build/object")
	targetdir ("%{wks.location}/.build/binaries/%{cfg.platform}/%{cfg.buildcfg}")
	
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
	include "vendor/ImGui"
group ""

group "Core"
	include "phoenix"
group ""

group "Misc"
	--include "Sandbox"
group ""


