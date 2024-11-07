file(GLOB PROTO_SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/gen_cpp/")
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(PROTO_CC_FILES)

foreach(proto_file ${PROTO_SRC_FILES})
    get_filename_component(proto_name ${proto_file} NAME_WE)
    set(pb_cc "${GENERATED_DIR}/${proto_name}.pb.cc")
    set(pb_h "${GENERATED_DIR}/${proto_name}.pb.h")

    add_custom_command(
        OUTPUT ${pb_cc} ${pb_h}
        COMMAND ${PROTOC_EXECUTABLE}
        ARGS --proto_path=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${GENERATED_DIR} ${proto_file}
        DEPENDS ${proto_file}
        COMMENT "Generating ${proto_name}.pb.cc and ${proto_name}.pb.h"
    )

    list(APPEND PROTO_CC_FILES ${pb_cc})
endforeach()

add_custom_target(GenerateProtoFiles ALL DEPENDS ${PROTO_CC_FILES})

add_library(proto_cc OBJECT ${PROTO_CC_FILES})

add_dependencies(proto_cc GenerateProtoFiles)

set(PROTO_LIB ${PROTO_LIB} proto_cc PARENT_SCOPE)
