add_library(suite_common STATIC
    include/suite/Brand.h
    include/suite/ParamSpec.h
    include/suite/dsp/Smoother.h
    include/suite/dsp/Svf.h
    include/suite/dsp/FFT.h
    include/suite/dsp/SpectrumAnalyzer.h
    include/suite/dsp/TruePeak.h
    include/suite/dsp/DelayLine.h
    include/suite/dsp/Oversampler.h
    include/suite/dsp/LoudnessMeter.h
    include/suite/vst/ProcessorBase.h
    include/suite/vst/ControllerBase.h
    include/suite/vst/SpecParameter.h
    include/suite/vst/AudioTap.h
    include/suite/vst/DynamicsTap.h
    include/suite/ui/Theme.h
    include/suite/ui/EditorBase.h
    include/suite/ui/Controls.h
    include/suite/ui/Meters.h

    source/ParamSpec.cpp
    source/dsp/SpectrumAnalyzer.cpp
    source/dsp/LoudnessMeter.cpp
    source/vst/ProcessorBase.cpp
    source/vst/ControllerBase.cpp
    source/vst/SpecParameter.cpp
    source/vst/AudioTap.cpp
    source/ui/Theme.cpp
    source/ui/EditorBase.cpp
    source/ui/Controls.cpp
    source/ui/Meters.cpp
)

target_include_directories(suite_common PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(suite_common PUBLIC sdk vstgui_support)
target_compile_features(suite_common PUBLIC cxx_std_17)
set_target_properties(suite_common PROPERTIES FOLDER "Suite")

if(MSVC)
    target_compile_options(suite_common PRIVATE /W3 /fp:fast)
endif()
