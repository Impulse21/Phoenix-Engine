
file(GLOB PhxEngineRenderer_Src
    Include/PhxEngine/Renderer/*.h
    Src/Renderer/*.h
    Src/Renderer/*.cpp
)

add_library(PhxEngineRenderer STATIC EXCLUDE_FROM_ALL ${PhxEngineRenderer_Src})

target_include_directories(PhxEngineRenderer PUBLIC Include)
target_link_libraries(PhxEngineRenderer PhxEngineCore PhxEngineRHI_Dx12 cgltf DirectXTex)

if(WIN32)
    target_compile_definitions(PhxEngineRenderer PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(PhxEngineRenderer PROPERTIES FOLDER PhxEngine)