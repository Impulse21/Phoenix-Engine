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


target_link_libraries(PhxEngine PUBLIC spdlog imgui entt cgltf DirectXTex)
set_target_properties(PhxEngine PROPERTIES FOLDER "Engine")

# DXC
message("Fetching DirectXShaderCompiler")

if (USING_MSVC)
    set(DXC_SDK_URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31.zip)
else()
    set(DXC_SDK_URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.8.2407/dxc_2024_07_31_clang_cl.zip)
endif()

FetchContent_Declare(
    dxc 
    URL https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212.1/dxc_2023_03_01.zip
    SOURCE_DIR ${CMAKE_BINARY_DIR}/dxc)

FetchContent_MakeAvailable(dxc)

file(MAKE_DIRECTORY ${dxc_SOURCE_DIR}/include)
file(COPY ${dxc_SOURCE_DIR}/inc/d3d12shader.h DESTINATION ${dxc_SOURCE_DIR}/include/dxc)
file(COPY ${dxc_SOURCE_DIR}/inc/dxcapi.h DESTINATION ${dxc_SOURCE_DIR}/include/dxc)
add_library(
	DirectXShaderCompiler
	INTERFACE
	"${dxc_SOURCE_DIR}/include/dxc/d3d12shader.h"
	"${dxc_SOURCE_DIR}/include/dxc/dxcapi.h"
	"${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll"
	"${dxc_SOURCE_DIR}/bin/x64/dxil.dll")
    
file(GLOB_RECURSE dxc_include "${dxc_SOURCE_DIR}/include")
set_target_properties(DirectXShaderCompiler  PROPERTIES FOLDER ${3rdparty_folder})


add_library(DirectXShaderCompiler SHARED IMPORTED)
set_target_properties(DirectXShaderCompiler PROPERTIES
    IMPORTED_LOCATION "${dxc_SOURCE_DIR}/bin/x64/dxcompiler.dll"
    IMPORTED_IMPLIB "${dxc_SOURCE_DIR}/lib/x64/dxcompiler.lib"
)

add_library(Dxil SHARED IMPORTED)
set_target_properties(Dxil PROPERTIES
    IMPORTED_LOCATION "${dxc_SOURCE_DIR}/bin/x64/dxil.dll"
    IMPORTED_IMPLIB "${dxc_SOURCE_DIR}/lib/x64/dxil.lib"
)

target_include_directories(PhxEngine PRIVATE 
    ${dxc_include}
)

target_link_libraries(PhxEngine PRIVATE DirectXShaderCompiler Dxil)

message("DirectXShaderCompiler fetched")

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
    message("D3D12 Agility SDK fetched")

    
    message("Fetching PIX")
    
    set(PIX_VERSION "1.0.240308001")
    string(CONFIGURE "https://www.nuget.org/api/v2/package/WinPixEventRuntime/${PIX_VERSION}" PIX_URL)

    FetchContent_Declare(
        pix 
        URL ${PIX_URL}
        SOURCE_DIR ${CMAKE_BINARY_DIR}/pix))
    FetchContent_MakeAvailable(pix)

    add_library(pix SHARED IMPORTED)
    set_target_properties(pix PROPERTIES
        IMPORTED_LOCATION "${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.dll"
        IMPORTED_IMPLIB "${pix_SOURCE_DIR}/bin/x64/WinPixEventRuntime.lib"
    )

    file(GLOB_RECURSE pix_include "${pix_SOURCE_DIR}/Include/WinPixEventRuntime/*.h")
    message("PIX fetched")

    target_include_directories(PhxEngine PRIVATE 
        ${d3d12-agility-sdk_include}
        ${pix_include}
    )
    target_link_libraries(PhxEngine PRIVATE AgilitySDK D3D12MemoryAllocator PIX)
endif()