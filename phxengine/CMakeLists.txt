include(FetchContent)

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

set(pch_file include/phx/pch.h)
set(pch_src src/pch.cpp)  # PCH.cpp typically just includes PCH.h
    
add_library(PhxEngine STATIC
    ${include_engine}
    ${include_core}
    ${include_renderer}
    ${include_rhi}
    ${src_engine}
    ${src_core}
    ${src_renderer}
    ${src_rhi})

    
source_group("include/phx"				FILES ${include_engine})
source_group("include/phx/core"         FILES ${include_core})
source_group("include/phx/renderer"     FILES ${include_renderer})
source_group("include/phx/rhi"          FILES ${include_rhi})

source_group("src"						FILES ${src_engine})
source_group("src/core"                 FILES ${src_core})
source_group("src/renderer"             FILES ${src_renderer})
source_group("src/rhi"                  FILES ${src_rhi})
    
if (USING_MSVC)
    target_precompile_headers(PhxEngine PRIVATE ${pch_file})
else()
    # Compile the PCH file
    add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/pch.gch
        COMMAND ${CMAKE_CXX_COMPILER} -o ${CMAKE_BINARY_DIR}/pch.gch -c ${pch_src}
        DEPENDS ${pch_src}
        COMMENT "Generating precompiled header"
    )

    # Add the precompiled header to the list of dependencies
    #add_dependencies(PhxEngine ${CMAKE_BINARY_DIR}/pch.gch)
    target_link_libraries(PhxEngine PRIVATE ${CMAKE_BINARY_DIR}/pch.gch)
endif()


target_include_directories(PhxEngine PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include>)


target_link_libraries(PhxEngine PUBLIC spdlog)
set_target_properties(PhxEngine PROPERTIES FOLDER "Engine")

if (PHX_ENABLE_DX12)
    
    target_sources(PhxEngine PRIVATE 
        ${include_rhi_dx12}
        ${src_rhi_d3d12})
    
    source_group("include/phx/rhi/dx12" FILES ${include_rhi_dx12})
    source_group("src/rhi/dx12"         FILES ${src_rhi_d3d12})
    
    
    message("Fetching D3D12 Agility SDK")
    set(D3D12_AGILITY_SDK_VERSION "1.715.1-preview")
    string(CONFIGURE "https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/${D3D12_AGILITY_SDK_VERSION}" D3D12_AGILITY_SDK_URL)

    FetchContent_Declare(
        d3d12-agility-sdk 
        URL ${D3D12_AGILITY_SDK_URL}
        SOURCE_DIR ${CMAKE_BINARY_DIR}/d3d12-agility-sdk)

    FetchContent_MakeAvailable(d3d12-agility-sdk)

    add_library(AgilitySDK SHARED IMPORTED)
    set_target_properties(AgilitySDK PROPERTIES
        IMPORTED_LOCATION "${d3d12-agility-sdk_SOURCE_DIR}/bin/D3D12Core.dll"
        IMPORTED_IMPLIB "${d3d12-agility-sdk_SOURCE_DIR}/lib/D3D12Core.lib"
    )

    file(GLOB_RECURSE d3d12-agility-sdk_include "${d3d12-agility-sdk_SOURCE_DIR}/build/native/include/*.h")
    target_include_directories(PhxEngine PRIVATE 
        ${d3d12-agility-sdk_include}
    )

    target_link_libraries(PhxEngine PRIVATE AgilitySDK)
endif()