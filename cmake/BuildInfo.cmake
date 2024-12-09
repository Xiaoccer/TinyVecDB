set(BUILDINFO_TEMPLATE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/buildinfo)
file(MAKE_DIRECTORY ${DESTINATION})

add_custom_target(generate_buildinfo ALL
    COMMAND bash ${BUILDINFO_TEMPLATE_DIR}/buildinfo.sh
                 ${BUILDINFO_TEMPLATE_DIR}/buildinfo.h.in
                 ${DESTINATION}/buildinfo.h
                 ${PROJECT_VERSION}
)

function(BuildInfo target)
    add_dependencies(${target} generate_buildinfo)
    target_include_directories(${target} PRIVATE ${DESTINATION})
endfunction()
