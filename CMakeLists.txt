cmake_minimum_required (VERSION 3.10...3.22)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(vdb_server VERSION 1.0 DESCRIPTION "TinyVecDB" LANGUAGES CXX)

file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" PATH_TO_CMAKELISTS_TXT)
if (EXISTS "${PATH_TO_CMAKELISTS_TXT}")
    message(FATAL_ERROR "Run CMake from a build subdirectory! \"mkdir build ; cd build ; cmake ..\"")
endif ()

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -Wextra -Werror -pedantic -pthread -O3")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS: ${CMAKE_EXE_LINKER_FLAGS}")
message(STATUS "CMAKE_SHARED_LINKER_FLAGS: ${CMAKE_SHARED_LINKER_FLAGS}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

##########################################
# Include
##########################################
set(VDB_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/vdb)
set(THIRD_PARTY_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/third_party/installed/include)
set(PROTOS_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/protos)

include_directories(${VDB_INCLUDE_DIR})
include_directories(SYSTEM ${THIRD_PARTY_INCLUDE_DIR} ${PROTOS_INCLUDE_DIR})

##########################################
# Thiry party libs && proto libs
##########################################
set(THIRD_PARTY_LIB)
set(PROTO_LIB)
set(PROTOC_EXECUTABLE ${PROJECT_SOURCE_DIR}/third_party/installed/bin/protoc)

add_subdirectory(third_party)
add_subdirectory(protos)
add_subdirectory(vdb)
