
# Required dependencies

file(GLOB PhxEngine_ModelViewer_SRC
    Src/*.h
    Src/*.cpp)

add_executable(PhxEngine-ModelViewer ${PhxEngine_ModelViewer_SRC})
target_link_libraries(PhxEngine-ModelViewer PhxEngine)

set_target_properties(PhxEngine-ModelViewer  PROPERTIES FOLDER "Applications/Model Viewer")
set_target_properties(PhxEngine-ModelViewer PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# Set the subsystem to "windows"
if(WIN32)
    target_link_options(PhxEngine-ModelViewer PRIVATE "/SUBSYSTEM:WINDOWS")
endif()
