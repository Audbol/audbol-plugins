cmake_minimum_required(VERSION 3.25)

# ---------------------------------------------------------------------------
# VST3 SDK configuration (must be set before the SDK is added)
# ---------------------------------------------------------------------------
set(vst3sdk_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/vst3sdk")
if(NOT EXISTS "${vst3sdk_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "VST3 SDK not found. Run: git submodule update --init --recursive")
endif()

set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
# ON builds the SDK's editorhost, handy for opening a plug-in UI without a DAW
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "Build SDK hosting examples (editorhost)")
# Symlinking into Common Files\VST3 is opt-in: -DSMTG_CREATE_PLUGIN_LINK=ON
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "Link built plug-ins into the system VST3 folder")

project(PluginSuite
    VERSION 0.1.0
    DESCRIPTION "Professional VST3 plug-in suite"
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(${vst3sdk_SOURCE_DIR} ${PROJECT_BINARY_DIR}/vst3sdk)
smtg_enable_vst3_sdk()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(SuitePlugin)

# ---------------------------------------------------------------------------
# Suite
# ---------------------------------------------------------------------------
add_subdirectory(common)
add_subdirectory(plugins/eq)
add_subdirectory(plugins/comp)
add_subdirectory(plugins/limiter)
add_subdirectory(plugins/reverb)
add_subdirectory(plugins/delay)
add_subdirectory(plugins/saturation)
add_subdirectory(plugins/chorus)
add_subdirectory(plugins/utility)
add_subdirectory(plugins/loudness)
add_subdirectory(plugins/scope)

# Tools
add_subdirectory(tools/bench)
