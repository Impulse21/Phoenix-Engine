@echo off
start /B /W vendor/7z/7z x vendor/PrebuiltLibs.7z -o.workspace/ -aoa
start /B /W vendor/premake/premake5 --file=PhxEngine.lua vs2022