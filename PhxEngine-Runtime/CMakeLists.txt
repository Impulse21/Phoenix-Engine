
# Required dependencies

file(GLOB PhxEngine_Runtime_SRC
    Src/*.h
    Src/*.cpp)

add_executable(PhxEngine-Runtime ${PhxEngine_Runtime_SRC})
target_link_libraries(PhxEngine-Runtime PhxEngine)

set_target_properties(PhxEngine-Runtime  PROPERTIES FOLDER "Applications/Runtime")
set_target_properties(PhxEngine-Runtime PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")