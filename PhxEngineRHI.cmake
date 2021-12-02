
file(GLOB PhxEngineRHI_Src
    Include/PhxEngine/RHI/*.h)

add_library(PhxEngineRHI INTERFACE ${PhxEngineRHI_Src})

set_target_properties(PhxEngineRHI PROPERTIES FOLDER "PhxEngine/RHI")

# Dx12 implementaion
file(GLOB PhxEngineRHI_Dx12Src
    Src/RHI/Dx12/*.h
    Src/RHI/Dx12/*.cpp
)

add_library(PhxEngineRHI_Dx12 STATIC EXCLUDE_FROM_ALL ${PhxEngineRHI_Dx12Src} ${PhxEngineRHI_Src})

target_include_directories(PhxEngineRHI_Dx12 PUBLIC Include)
target_link_libraries(
    PhxEngineRHI_Dx12
    PhxEngineCore
    DirectX-Headers
    DirectX-Guids
    PhxEngineRHI
    d3d12.lib
    dxgi.lib)

if(WIN32)
    target_compile_definitions(PhxEngineRHI_Dx12 PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

set_target_properties(PhxEngineRHI_Dx12 PROPERTIES FOLDER "PhxEngine/RHI")