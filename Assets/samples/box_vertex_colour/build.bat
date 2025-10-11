@echo off
REM Sets the working directory to the location of this batch script.
echo Changing directory to script location...
pushd "%~dp0"

echo Current working directory: %cd%
echo.

set PYTHON_SCRIPT_PATH="../../../scripts/asset_pipeline/build_asset.py"
set BUILD_FILE_PATH="build_config.json"

echo Executing Python script: %PYTHON_SCRIPT_PATH%
echo ------------------------------------------
python %PYTHON_SCRIPT_PATH% %BUILD_FILE_PATH%
echo ------------------------------------------
echo.
echo Python script finished.

REM Returns to the original directory you were in before running the script.
popd

pause
