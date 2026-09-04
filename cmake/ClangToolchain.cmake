set(CMAKE_C_COMPILER   clang)
set(CMAKE_CXX_COMPILER clang++)

# On Windows, point at the LLVM install if not on PATH
if(WIN32)
    # Adjust to your LLVM path if needed
    # set(CMAKE_C_COMPILER   "C:/Program Files/LLVM/bin/clang.exe")
    # set(CMAKE_CXX_COMPILER "C:/Program Files/LLVM/bin/clang++.exe")
endif()