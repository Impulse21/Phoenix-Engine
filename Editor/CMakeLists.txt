
# Required dependencies

file(GLOB Engine_Src
    Include/*.h
    Src/*.h
    Src/*.cpp)

add_executable(Editor ${Engine_Src})
target_link_libraries(Editor PhxEngine PhxEngineShaders)

target_include_directories(Editor PRIVATE ../Shaders/Include)


set_property(TARGET  PROPERTY FOLDER "Applications")
set_target_properties(Editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")