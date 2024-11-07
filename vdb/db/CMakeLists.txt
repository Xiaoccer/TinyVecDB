add_library(
        vdb_db
        OBJECT
        database.cc)

add_dependencies(vdb_db ${PROTO_LIB})

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vdb_db>
        PARENT_SCOPE)
