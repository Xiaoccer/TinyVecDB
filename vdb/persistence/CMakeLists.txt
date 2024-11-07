add_library(
        vdb_persistence
        OBJECT
        kv_storage.cc
        persistence.cc)

set(ALL_OBJECT_FILES
        ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:vdb_persistence>
        PARENT_SCOPE)
