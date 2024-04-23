file(GLOB sources "Src/*.cpp" "Src/*.h")

set(project PhxShaderCompiler)
set(folder "Applications/Tools")
set(extern_folder_name "Extern")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
add_executable(${project} WIN32 ${sources})
target_link_libraries(${project} PUBLIC PhxEngine DirectXShaderCompiler yaml-cpp::yaml-cpp)
set_target_properties(${project} PROPERTIES FOLDER "${folder}")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

