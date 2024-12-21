@echo off
start /B /W premake/premake5 --file=phxengine.lua vs2022
rem cd .vsfiles
rem PhxEngine.sln
echo Build finished succesfully
pause