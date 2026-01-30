@echo off
REM Build script for Phoenix Engine on Windows

echo Phoenix Engine - Windows Build Script
echo ====================================
echo.

REM Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake not found. Please install CMake 3.28 or later.
    exit /b 1
)

REM Parse arguments
set BUILD_TYPE=%1
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Debug

set BUILD_DIR=build

echo Configuration:
echo   Generator: Visual Studio 17 2022
echo   Build Type: %BUILD_TYPE%
echo   Build Directory: %BUILD_DIR%
echo.

REM Create build directory
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure
echo Configuring...
cmake -G "Visual Studio 17 2022" -A x64 ..
if %ERRORLEVEL% NEQ 0 (
    echo Configuration failed!
    exit /b 1
)

REM Build
echo Building...
cmake --build . --config %BUILD_TYPE% -j
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build complete!
echo Executables are in: %BUILD_DIR%\bin\%BUILD_TYPE%\
echo.
echo To run PhxEditor:
echo   %BUILD_DIR%\bin\%BUILD_TYPE%\PhxEditor.exe
echo.
pause
