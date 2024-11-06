# set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wall -Wextra -Werror -pedantic -pthread -O3")

set(GENSRC_DIR "${PROJECT_SOURCE_DIR}/proto/build")

include_directories(
    SYSTEM
    ${PROJECT_SOURCE_DIR}/proto/build/
    ${PROJECT_SOURCE_DIR}/third_party/installed/include
)

# Source files
set(SOURCES
    index_factory.cc
    index.cc
    field_bitmap.cc
    vdb_service.cc
    vdb_server.cc
    kv_storage.cc
    persistence.cc
    database.cc
    main.cc
)

set(WL_START_GROUP "-Wl,--start-group")
set(WL_END_GROUP "-Wl,--end-group")

# -lfaiss -fopenmp -lopenblas -lpthread -lspdlog

set(LINK_LIBS ${LINK_LIBS}
    ${WL_START_GROUP}
    -lfaiss
    -lopenblas
    -fopenmp
    -lrocksdb
    -lsnappy
    -llz4
    -lbz2
    -lbrpc
    -lglog
    -lgflags
    -lprotobuf
    -lcrypto
    -lleveldb
    -lssl
    -lz
    -lroaring
    -lpthread
    ${WL_END_GROUP}
    -lstdc++fs
    -lgfortran
    -ldl
)

# Include directories
#include_directories(${PROJECT_SOURCE_DIR}/src)
# include_directories(SYSTEM ${PROJECT_SOURCE_DIR}/third_party/installed/include)

add_library(faiss STATIC IMPORTED)
set_target_properties(faiss PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libfaiss.a)
add_library(openblas STATIC IMPORTED)
set_target_properties(openblas PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libopenblas.a)
add_library(brpc STATIC IMPORTED)
set_target_properties(brpc PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libbrpc.a)
add_library(gflags STATIC IMPORTED)
set_target_properties(gflags PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libgflags.a)
add_library(protobuf STATIC IMPORTED)
set_target_properties(protobuf PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libprotobuf.a)
add_library(glog STATIC IMPORTED)
set_target_properties(glog PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libglog.a)
add_library(crypto STATIC IMPORTED)
set_target_properties(crypto PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libcrypto.a)
add_library(leveldb STATIC IMPORTED)
set_target_properties(leveldb PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libleveldb.a)
add_library(ssl STATIC IMPORTED)
set_target_properties(ssl PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libssl.a)
add_library(z STATIC IMPORTED)
set_target_properties(z PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libz.a)
add_library(rocksdb STATIC IMPORTED)
set_target_properties(rocksdb PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/librocksdb.a)
add_library(snappy STATIC IMPORTED)
set_target_properties(snappy PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libsnappy.a)
add_library(lz4 STATIC IMPORTED)
set_target_properties(lz4 PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/liblz4.a)
add_library(bz2 STATIC IMPORTED)
set_target_properties(bz2 PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libbz2.a)
add_library(roaring STATIC IMPORTED)
set_target_properties(roaring PROPERTIES IMPORTED_LOCATION ${PROJECT_SOURCE_DIR}/third_party/installed/lib/libroaring.a)

#set_target_properties(faiss PROPERTIES
#    INTERFACE_INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/third_party/include"
#)

add_library(proto_lib ${PROJECT_SOURCE_DIR}/proto/build/gen_cpp/vdb.pb.cc ${PROJECT_SOURCE_DIR}/proto/build/gen_cpp/vdb.pb.h)
# Create executable

add_executable(vdb_server ${SOURCES})
target_include_directories(vdb_server PRIVATE ${PROJECT_SOURCE_DIR}/src/)
target_compile_options(vdb_server PRIVATE -Wall -Wextra -Werror -pedantic -pthread  -O3)
target_link_directories(vdb_server PRIVATE ${PROJECT_SOURCE_DIR}/third_party/installed/lib)
target_link_libraries(vdb_server PRIVATE proto_lib ${LINK_LIBS})

#target_include_directories(vdb_server SYSTEM PRIVATE ${PROJECT_SOURCE_DIR}/third_party/installed/include)
# Link libraries (add any specific libraries required)
# target_link_libraries(vdb_server PRIVATE ${LINK_LIBS})
