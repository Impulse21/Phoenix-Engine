
# Required dependencies

file(GLOB PhxEngine_Editor_SRC
    Src/*.h
    Src/*.cpp)

add_executable(PhxEngine-Editor ${PhxEngine_Editor_SRC})
target_link_libraries(PhxEngine-Editor PhxEngine)

set_target_properties(PhxEngine-Editor  PROPERTIES FOLDER "Applications/Editor")
set_target_properties(PhxEngine-Editor PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")