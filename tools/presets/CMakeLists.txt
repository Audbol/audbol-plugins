# make_presets: writes the factory presets as .vstpreset files. It compiles
# in each plug-in's parameter and preset tables, so the files always match
# what the plug-ins' preset browsers load.
set(P "${PROJECT_SOURCE_DIR}/plugins")
add_executable(make_presets
    make_presets.cpp
    ${P}/eq/source/EqParams.cpp         ${P}/eq/source/EqPresets.cpp
    ${P}/comp/source/CompParams.cpp     ${P}/comp/source/CompPresets.cpp
    ${P}/limiter/source/LimParams.cpp   ${P}/limiter/source/LimPresets.cpp
    ${P}/reverb/source/RevParams.cpp    ${P}/reverb/source/RevPresets.cpp
    ${P}/delay/source/DlyParams.cpp     ${P}/delay/source/DlyPresets.cpp
    ${P}/saturation/source/SatParams.cpp ${P}/saturation/source/SatPresets.cpp
    ${P}/chorus/source/ChoParams.cpp    ${P}/chorus/source/ChoPresets.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vstpresetfile.cpp
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/common/memorystream.cpp
)
target_include_directories(make_presets PRIVATE
    ${P}/eq/source ${P}/comp/source ${P}/limiter/source ${P}/reverb/source
    ${P}/delay/source ${P}/saturation/source ${P}/chorus/source
)
target_link_libraries(make_presets PRIVATE suite_common)
target_compile_features(make_presets PRIVATE cxx_std_17)
set_target_properties(make_presets PROPERTIES FOLDER "Tools")
