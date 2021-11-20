
file(GLOB PhxEngineCore_Src
    Include/PhxEngine/Core/*.h
    Src/Core/*.cpp
)

add_library(PhxEngineCore STATIC EXCLUDE_FROM_ALL ${PhxEngineCore_Src})

target_include_directories(PhxEngineCore PUBLIC Include)
target_link_libraries(PhxEngineCore spdlog)

if(WIN32)
    target_compile_definitions(PhxEngineCore PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(PhxEngineCore PROPERTIES FOLDER PhxEngine)