cmake_minimum_required(VERSION 3.16)
project(evo_sim C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RAYLIB REQUIRED IMPORTED_TARGET raylib)

add_executable(evo_sim main.cpp )
target_link_libraries(evo_sim PRIVATE PkgConfig::RAYLIB)

if(WIN32)
    target_link_libraries(evo_sim PRIVATE winmm gdi32 opengl32)
endif()
