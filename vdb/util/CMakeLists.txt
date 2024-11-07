add_library(
        vdb_util
        OBJECT
        util.h)

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vdb_util>
        PARENT_SCOPE)
