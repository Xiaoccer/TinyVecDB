add_subdirectory(bitmap)
add_subdirectory(db)
add_subdirectory(index)
add_subdirectory(persistence)
add_subdirectory(server)
add_subdirectory(util)

##########################################
# vdb lib
##########################################
add_library(vdb STATIC ${ALL_OBJECT_FILES})

set(VDB_LIBS
        vdb_bitmap
        vdb_db
        vdb_index
        vdb_persistence
        vdb_server
        vdb_util
    )
set(WL_START_GROUP "-Wl,--start-group")
set(WL_END_GROUP "-Wl,--end-group")
set(LINK_LIBS ${LINK_LIBS}
    ${WL_START_GROUP}
    ${THIRD_PARTY_LIB}
    ${WL_END_GROUP}
    ${PROTO_LIB}
    ${VDB_LIBS}
    -fopenmp
    -lpthread
    -lstdc++fs
    -lgfortran
    -ldl
)
message(STATUS "Link_libs: ${LINK_LIBS}")
target_link_libraries(vdb ${LINK_LIBS})

##########################################
# main
##########################################
set(SOURCES main.cc)
add_executable(main ${SOURCES})
target_link_libraries(main vdb)
set_target_properties(main PROPERTIES OUTPUT_NAME server)
