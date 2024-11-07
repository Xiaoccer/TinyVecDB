add_library(
        vdb_bitmap
        OBJECT
        field_bitmap.cc)

add_dependencies(vdb_bitmap ${PROTO_LIB})

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vdb_bitmap>
        PARENT_SCOPE)
