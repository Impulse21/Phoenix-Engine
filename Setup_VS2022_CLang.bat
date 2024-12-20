@echo off
rem if not exist ".build" mkdir .build
rem cd .build

rem cmake -G "Visual Studio 17 2022" -A x64 ../  -T ClangCL
cmake --preset x64-Debug-Clang-Vs2022
pause
