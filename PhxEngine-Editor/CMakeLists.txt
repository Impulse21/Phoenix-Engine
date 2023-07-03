include(../PhxEngine/compileshaders.cmake)

file(GLOB sources "Src/*.cpp" "Src/*.h")

set(project PhxEngine-Editor)
set(folder "Applications/Editor")

add_executable(${project} WIN32 ${sources})

target_link_libraries(${project} PhxEngine)
set_target_properties(${project} PROPERTIES FOLDER ${folder})

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /MP")
endif()