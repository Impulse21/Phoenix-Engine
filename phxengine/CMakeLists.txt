
file(GLOB include_engine
    include/phx/*.h)
file(GLOB src_engine
    src/*.cpp
    src/*.h)

file(GLOB include_core
    include/phx/core/*.h)
file(GLOB src_core
    src/core/*.cpp
    src/core/*.h)
    
file(GLOB include_renderer
    include/phx/renderer/*.h)
file(GLOB src_renderer
    src/renderer/*.cpp
    src/renderer/*.h)
    
file(GLOB include_rhi
    include/phx/rhi/*.h)
file(GLOB src_rhi
    src/rhi/*.cpp
    src/rhi/*.h)

file(GLOB include_rhi_dx12
    include/phx/rhi/dx12/*.h)
file(GLOB src_rhi_d3d12
    src/rhi/dx12/*.cpp
    src/rhi/dx12/*.h)
    
file(GLOB include_rhi_vk
    include/phx/rhi/vk/*.h)
file(GLOB src_rhi_vk
    src/rhi/vk/*.cpp
    src/rhi/vk/*.h)

    
add_library(PhxEngine STATIC
    ${include_engine}
    ${include_core}
    ${include_renderer}
    ${include_rhi}
    ${src_engine}
    ${src_core}
    ${src_renderer}
    ${src_rhi})

    
target_include_directories(PhxEngine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)


set_target_properties(PhxEngine PROPERTIES FOLDER "Engine")

if (PHX_ENABLE_DX12)
    
    target_sources(PhxEngine PRIVATE 
        ${include_rhi_dx12}
        ${src_rhi_d3d12})
    
    target_link_libraries(PhxEngine PUBLIC d3d12 dxguid)

endif()