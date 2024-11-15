add_library(
        vdb_server
        OBJECT
        server.cc
        service.cc)

add_dependencies(vdb_server ${PROTO_LIB})

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vdb_server>
        PARENT_SCOPE)
