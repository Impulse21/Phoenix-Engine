@echo off
if not exist "Build" mkdir Build
cd Build

cmake -G "Visual Studio 17 2022" -A x64 ../
pause
