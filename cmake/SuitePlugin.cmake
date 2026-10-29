# suite_add_plugin(<target> [DISPLAY "<name>"] SOURCES <files...>)
#
# Creates a VST3 plug-in target wired to the shared suite library. The SDK
# runs its validator after every build, so a plug-in that fails the VST3
# conformance tests fails the build.
function(suite_add_plugin target)
    cmake_parse_arguments(ARG "" "DISPLAY" "SOURCES" ${ARGN})

    # Audbol icon on the .vst3 bundle folder (the SDK reads this variable)
    set(SMTG_PACKAGE_ICON_PATH "${PROJECT_SOURCE_DIR}/resources/brand/audbol.ico")

    smtg_add_vst3plugin(${target} ${ARG_SOURCES})
    smtg_target_configure_version_file(${target})

    # Version resource: Explorer > Properties shows product, company, version
    if(WIN32)
        set(SUITE_PLUGIN_TARGET ${target})
        if(ARG_DISPLAY)
            set(SUITE_PLUGIN_DISPLAY "${ARG_DISPLAY}")
        else()
            string(REPLACE "Audbol" "Audbol " SUITE_PLUGIN_DISPLAY ${target})
        endif()
        configure_file("${PROJECT_SOURCE_DIR}/cmake/version.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/version.rc" @ONLY)
        target_sources(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
    endif()

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
