# suite_add_plugin(<target> SOURCES <files...>)
#
# Creates a VST3 plug-in target wired to the shared suite library. The SDK
# runs its validator after every build, so a plug-in that fails the VST3
# conformance tests fails the build.
function(suite_add_plugin target)
    cmake_parse_arguments(ARG "" "" "SOURCES" ${ARGN})

    smtg_add_vst3plugin(${target} ${ARG_SOURCES})
    smtg_target_configure_version_file(${target})

    target_link_libraries(${target} PRIVATE suite_common)
    # projectversion.h is generated into the current binary dir
    target_include_directories(${target} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/source"
        "${CMAKE_CURRENT_BINARY_DIR}"
    )
    set_target_properties(${target} PROPERTIES FOLDER "Plugins")

    if(MSVC)
        target_compile_options(${target} PRIVATE /W3 /fp:fast)
    endif()
endfunction()
