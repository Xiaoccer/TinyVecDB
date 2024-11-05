cmake_minimum_required (VERSION 3.10...3.22)

project(vdb_server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build src
set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")
add_subdirectory(${SRC_DIR})
