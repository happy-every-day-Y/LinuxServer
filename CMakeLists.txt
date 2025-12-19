cmake_minimum_required(VERSION 3.20)

project(LinuxServer
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # 给 clangd / nvim 用

add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

add_library(project_options INTERFACE)
target_compile_options(project_options INTERFACE
    -Wall
    -Wextra
    -Wpedantic
)

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(apps)