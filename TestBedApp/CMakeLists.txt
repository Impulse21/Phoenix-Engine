
# Required dependencies

add_executable(TestBed Src/Main.cpp)
target_link_libraries(TestBed PhxEngineCore)

set_property(TARGET  PROPERTY FOLDER "Applications")