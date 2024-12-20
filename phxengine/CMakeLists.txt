
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
    
target_compile_options(PhxEngine PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic -Werror>
)

target_compile_options(PhxEngine PRIVATE ${COMMON_COMPILE_OPTIONS})

include(CheckCXXCompilerFlag)

check_cxx_compiler_flag("-Wno-unsafe-buffer-usage" HAS_UNSAFE_BUFFER_FLAG)

if(HAS_UNSAFE_BUFFER_FLAG)
    message(STATUS "Compiler supports -Wno-unsafe-buffer-usage")
    add_compile_options(-Wno-unsafe-buffer-usage)
else()
    message(STATUS "Compiler does NOT support -Wno-unsafe-buffer-usage")
endif()

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

set_target_properties(PhxEngine PROPERTIES FOLDER "Libs")

target_include_directories(PhxEngine PRIVATE ${dxc_include})

target_link_libraries(PhxEngine PUBLIC spdlog_header_only  imgui entt cgltf DirectXTex)
target_link_libraries(PhxEngine PUBLIC DirectXShaderCompiler Dxil)

message("DirectXShaderCompiler fetched")
#[===[
if (IS_WINDOWS)

    # Include GDK toolchain
    set(GDK_INCLUDE_DIR "$ENV{GXDKLatest}\\Include")
    set(GDK_LIB_DIR "$ENV{GXDKLatest}\\Lib\\$(Platform)\\$(Configuration)")

    target_include_directories(PhxEngine SYSTEM PRIVATE ${GDK_INCLUDE_DIR})
    target_link_directories(PhxEngine PRIVATE ${GDK_LIB_DIR})

    # Link against the required GDK libraries
    target_link_libraries(PhxEngine 
        xgame 
        xsystem 
        xgraphics 
        xaudio2 
        xinput
    )
endif()
]===]

if (PHX_ENABLE_DX12)
    
    target_sources(PhxEngine PRIVATE 
        ${include_rhi_dx12}
        ${src_rhi_d3d12}
    )
    
    target_include_directories(PhxEngine PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/phx/rhi/dx12/>
        $<INSTALL_INTERFACE:$<INSTALL_PREFIX>/include/phx/rhi/dx12/>
    )

    source_group("include/phx/rhi/dx12" FILES ${include_rhi_dx12})
    source_group("src/rhi/dx12"         FILES ${src_rhi_d3d12})
        
    target_include_directories(PhxEngine SYSTEM PUBLIC 
        ${d3d12-agility-sdk_include}
        ${pix_include}
        ${include_rhi_dx12}
    )

    target_link_libraries(PhxEngine PUBLIC  D3D12Core D3D12MemoryAllocator WinPixEventRuntime)
    target_include_directories(PhxEngine SYSTEM PUBLIC 
        $<TARGET_PROPERTY:D3D12MemoryAllocator,INTERFACE_INCLUDE_DIRECTORIES>
    )
    target_compile_definitions(PhxEngine PUBLIC USING_D3D12_AGILITY_SDK PIX)
endif()