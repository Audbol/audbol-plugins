# Headless CPU benchmark for the suite's plug-ins (Windows).
if(NOT SMTG_WIN)
    return()
endif()

add_executable(bench
    bench.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/plugprovider.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting/module_win32.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/samples/vst-hosting/editorhost/source/usediids.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vstpresetfile.cpp
)
target_link_libraries(bench PRIVATE sdk_hosting)
target_compile_features(bench PRIVATE cxx_std_17)
set_target_properties(bench PROPERTIES FOLDER "Tools")
