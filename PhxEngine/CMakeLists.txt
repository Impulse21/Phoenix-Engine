cmake_minimum_required(VERSION 3.16)

# set the project name and version
set(project PhxEngine)

project(${project})

set(folder "PhxEngine")

set_property( GLOBAL PROPERTY USE_FOLDERS ON)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -D_DEBUG")

include(CMakeDependentOption)

set(extern_folder "PhxEngine/External Libraries")

#-- Root includes ---
file (GLOB PhxEngine_Inc
    Include/PhxEngine/*.h)
	
file (GLOB PhxEngine_Src
    Src/*.h
	Src/*.cpp)

file (GLOB PhxEngine_Core_Inc
    Include/PhxEngine/Core/*.h)
	
file (GLOB PhxEngine_Core_Src
    Src/Core/*.h
	Src/Core/*.cpp)
	
file (GLOB PhxEngine_Renderer_Inc
    Include/PhxEngine/Renderer/*.h)
	
file (GLOB PhxEngine_Renderer_Src
    Src/Renderer/*.h
	Src/Renderer/*.cpp)
    
add_library(PhxEngine STATIC EXCLUDE_FROM_ALL  
    ${PhxEngine_Engine_Inc}
    ${PhxEngine_Engine_Src}
    ${PhxEngine_Core_Inc}
    ${PhxEngine_Core_Src}
    ${PhxEngine_Assets_Inc}
    ${PhxEngine_Assets_Src}
	${PhxEngine_Resource_Inc}
	${PhxEngine_Resource_Src}
    ${PhxEngine_Graphics_RHI_Inc}
    ${PhxEngine_Graphics_RHI_Src}
	${PhxEngine_Graphics_RHI_Windows_Src}
    ${PhxEngine_Graphics_RHI_DX12_Src}
    ${PhxEngine_Graphics_ImGui_Inc}
    ${PhxEngine_Graphics_ImGui_Src}
	${PhxEngine_Renderer_Inc}
	${PhxEngine_Renderer_Src}
	${PhxEngine_Renderer_DX12_Src}
	${PhxEngine_Shader_Interop_Inc}
    ${PhxEngine_Inc}
    ${PhxEngine_Src})

source_group("Include"					FILES ${PhxEngine_Inc})
source_group("Include/Core"				FILES ${PhxEngine_Core_Inc})
source_group("Include/Renderer"			FILES ${PhxEngine_Renderer_Inc})

source_group("Src"						FILES ${PhxEngine_Src})
source_group("Src/Core"					FILES ${PhxEngine_Core_Src})
source_group("Src/Core"					FILES ${PhxEngine_Renderer_Src})

target_include_directories(${project} 
	PUBLIC  "Include"
	PRIVATE "Src")

add_subdirectory(Extern)

target_link_libraries(${project} PUBLIC
    OptickCore
    json
    glfw)

set_target_properties(${project} PROPERTIES FOLDER ${folder})
