file(GLOB sources "Src/*.cpp" "Src/*.h")

set(project PhxAssetConverter)
set(folder "Applications/Tools")
set(extern_folder_name "Extern")

add_library(cgltf INTERFACE 
    ${CMAKE_CURRENT_SOURCE_DIR}/${extern_folder_name}/cgltf/cgltf.h)
target_include_directories(cgltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${extern_folder_name}/cgltf)
set_target_properties(cgltf PROPERTIES FOLDER ${extern_folder_name})

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${extern_folder_name}/meshoptimizer)
set_target_properties(meshoptimizer PROPERTIES FOLDER "${folder}/${extern_folder_name}")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/${extern_folder_name}/DirectXTex)
set_target_properties(DirectXTex PROPERTIES FOLDER "${folder}/${extern_folder_name}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")
add_executable(${project} WIN32 ${sources})
target_link_libraries(${project} PhxEngine cgltf meshoptimizer DirectXTex DirectStorageSDK)
set_target_properties(${project} PROPERTIES FOLDER "${folder}")

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()
