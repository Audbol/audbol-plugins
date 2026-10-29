# Headless CPU benchmark and offline renderer for the suite's plug-ins.
set(H "${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/hosting")
if(SMTG_WIN)
    set(BENCH_MODULE "${H}/module_win32.cpp")
elseif(SMTG_MAC)
    set(BENCH_MODULE "${H}/module_mac.mm")
else()
    set(BENCH_MODULE "${H}/module_linux.cpp")
endif()

add_executable(bench
    bench.cpp
    ${H}/plugprovider.cpp
    ${BENCH_MODULE}
    ${vst3sdk_SOURCE_DIR}/public.sdk/samples/vst-hosting/editorhost/source/usediids.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vstpresetfile.cpp
)
target_link_libraries(bench PRIVATE sdk_hosting)
if(SMTG_MAC)
    target_link_libraries(bench PRIVATE "-framework Foundation" "-framework CoreFoundation")
    target_compile_options(bench PRIVATE "-fobjc-arc")
elseif(SMTG_LINUX)
    target_link_libraries(bench PRIVATE ${CMAKE_DL_LIBS})
endif()
target_compile_features(bench PRIVATE cxx_std_17)
set_target_properties(bench PROPERTIES FOLDER "Tools")
