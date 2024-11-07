add_library(
        vdb_index
        OBJECT
        index.cc
        index_factory.cc)

add_dependencies(vdb_index ${PROTO_LIB})

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vdb_index>
        PARENT_SCOPE)
