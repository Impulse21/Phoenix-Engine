setlocal

:: Directories
set SHADER_DIR=..\Phoenix\renderer\shaders\precompiled\
set OUTPUT_DIR=..\.workspace\Generated\Shaders
set DXC_DIR=..\.workspace\PrebuiltLibs\dxc_2024_07_31_clang_cl\bin\x64\
set MERGED_HEADER=..\Phoenix\renderer\shaders\PrecompiledShaders.h

:: Create output directories if they don't exist
if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

:: Clear the merged header file
echo // Merged shader headers > "%MERGED_HEADER%"

:: Loop through each .hlsl file in the shader directory
for %%F in (%SHADER_DIR%\*.hlsl) do (
    :: Get the filename without the extension
   set "SHADER_NAME=%%~nF"
    set "SHADER_PROFILE="

    :: Check if the file name contains "PS" or "VS"
    echo %%~nF | findstr /I "PS" > nul
    if not errorlevel 1 (
        set "SHADER_PROFILE=ps_6_6"
    )

    echo %%~nF | findstr /I "VS" > nul
    if not errorlevel 1 (
        set "SHADER_PROFILE=vs_6_6"
    )

    :: Skip files that don't match any known profile
    if "%SHADER_PROFILE%"=="" (
        echo Skipping %%F: Unknown shader type
        goto :continue
    )

    :: Compile the shader using DXC
    %DXC_DIR%dxc.exe %%F -T %SHADER_PROFILE% -E main /Fo "%OUTPUT_DIR%\%%~nF.cso" /Fc "%OUTPUT_DIR%\%%~nF.h"

    :: Append the generated header file to the merged header file
    echo // Begin %%~nF.h >> "%MERGED_HEADER%"
    type "%OUTPUT_DIR%\%%~nF.h" >> "%MERGED_HEADER%"
    echo. >> "%MERGED_HEADER%"  :: Add a blank line for separation
    echo // End %%~nF.h >> "%MERGED_HEADER%"
)

echo All shaders compiled and merged into "%MERGED_HEADER%".
pause