
file(GLOB PhxEngineApp_Src
    Include/PhxEngine/App/*.h
    Src/App/*.cpp
)

add_library(PhxEngineApp STATIC EXCLUDE_FROM_ALL ${PhxEngineApp_Src})

target_include_directories(PhxEngineApp PUBLIC Include)
target_link_libraries(PhxEngineApp PhxEngineCore glfw)

if(WIN32)
    target_compile_definitions(PhxEngineApp PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(PhxEngineApp PROPERTIES FOLDER PhxEngine)