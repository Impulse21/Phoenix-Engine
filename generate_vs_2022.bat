@echo off
REM Quick Visual Studio 2022 Generation
REM Double-click this file to generate and open the solution

echo Generating Visual Studio 2022 solution...
cmake --preset windows-vs-2022

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Success! Opening Visual Studio...
    start "" ".build\vs2022\PhxEngine.sln"
) else (
    echo.
    echo Error: Generation failed. See generate_vs_2022.bat for detailed version.
    pause
)