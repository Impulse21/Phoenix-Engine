# set the project name and version
project(PhxEngineShaders VERSION 1.0)

# Shader Projects
set(shader_source_vs
    Src/PbrDemoVS.hlsl
    Src/DepthPathVS.hlsl
    Src/DebugVS.hlsl
    Src/MeshRender_CommonVS.hlsl
    Src/MeshRender_PrepassVS.hlsl
    Src/ImGuiVS.hlsl
    Src/GeometryPassVS.hlsl)
    
set(shader_source_ps
    Src/PbrDemoPS.hlsl
    Src/DebugPS.hlsl
    Src/MeshRender_BrdfPS.hlsl
    Src/ImGuiPS.hlsl
    Src/GeometryPassPS.hlsl)

set(shader_headers
    Src/BRDFFunctions.hlsli
    Src/Defines.hlsli
    Src/Globals.hlsli
    Src/MeshRender.hlsli
    Src/ResourceHeapTables.hlsli)

set(shader_source_includes
    Include/Shaders/DepthPathVS_compiled.h
    Include/Shaders/PbrDemoVS_compiled.h
    Include/Shaders/PbrDemoPS_compiled.h
    Include/Shaders/DebugVS_compiled.h
    Include/Shaders/DebugPS_compiled.h
    Include/Shaders/MeshRender_CommonVS_compiled.h
    Include/Shaders/MeshRender_PrepassVS_compiled.h
    Include/Shaders/MeshRender_BrdfPS_compiled.h
    Include/Shaders/ImGuiVS_compiled.h
    Include/Shaders/ImGuiPS_compiled.h
    Include/Shaders/GeometryPassVS_compiled.h
    Include/Shaders/GeometryPassPS_compiled.h)

set(shader_interop_includes
    Include/Shaders/ShaderInterop.h
    Include/Shaders/ShaderInteropStructures.h)

# first is extra debug flags
set_source_files_properties(${shader_source_vs} PROPERTIES VS_SHADER_FLAGS "/Fh \"${PROJECT_SOURCE_DIR}/Include/Shaders/%(Filename)_compiled.h\" /Vn \"g%(Filename)\" /Zi /Od /Qembed_debug" VS_SHADER_TYPE Vertex VS_SHADER_MODEL 6.5 VS_SHADER_ENTRYPOINT main)
#set_source_files_properties(${shader_source_vs} PROPERTIES VS_SHADER_FLAGS "/Fh \"${PROJECT_SOURCE_DIR}/Include/Shaders/%(Filename)_compiled.h\" /Vn \"g%(Filename)\" /Zi" VS_SHADER_TYPE Vertex VS_SHADER_MODEL 6.5 VS_SHADER_ENTRYPOINT main)

# first is extra debug flags
set_source_files_properties(${shader_source_ps} PROPERTIES VS_SHADER_FLAGS "/Fh \"${PROJECT_SOURCE_DIR}/Include/Shaders/%(Filename)_compiled.h\" /Vn \"g%(Filename)\" /Zi /Od /Qembed_debug" VS_SHADER_TYPE Pixel  VS_SHADER_MODEL 6.5 VS_SHADER_ENTRYPOINT main)
#set_source_files_properties(${shader_source_ps} PROPERTIES VS_SHADER_FLAGS "/Fh \"${PROJECT_SOURCE_DIR}/Include/Shaders/%(Filename)_compiled.h\" /Vn \"g%(Filename)\" /Zi" VS_SHADER_TYPE Pixel  VS_SHADER_MODEL 6.5 VS_SHADER_ENTRYPOINT main)

add_library(PhxEngineShaders INTERFACE 
    ${shader_source_vs}
    ${shader_source_ps}
    ${shader_headers}
    ${shader_source_includes}
    ${shader_interop_includes})
    