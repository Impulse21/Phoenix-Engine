
# set the project name and version
project(PhxEngineShaders VERSION 1.0)

# Shader Projects
set(shader_source_vs
    Src/PbrDemoVS.hlsl)
    
set(shader_source_ps
    Src/PbrDemoPS.hlsl)

set(shader_headers
    Src/BRDFFunctions.hlsli
    Src/Defines.hlsli)


set(shader_source_includes
    Include/Shaders/PbrDemoVS_compiled.h
    Include/Shaders/PbrDemoPS_compiled.h)

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
    ${shader_source_includes})
    