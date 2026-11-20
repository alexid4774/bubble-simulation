cmake_minimum_required(VERSION 3.16)

project(BubbleSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_path(GMSH_INCLUDE_DIR NAMES gmsh.h PATHS /opt/homebrew/opt/gmsh/include REQUIRED)

find_library(GMSH_LIBRARY NAMES gmsh PATHS /opt/homebrew/opt/gmsh/lib REQUIRED)

add_executable(mesh_generator geometry/src/main.cpp)

target_include_directories(mesh_generator PRIVATE ${GMSH_INCLUDE_DIR})

target_link_libraries(mesh_generator PRIVATE ${GMSH_LIBRARY})