cmake_minimum_required(VERSION 3.22)
project(CloudsVST VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Fetch JUCE ---

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
)
FetchContent_MakeAvailable(JUCE)

# --- Clouds DSP sources from eurorack submodule ---

set(CLOUDS_DSP_SOURCES
    libs/eurorack/clouds/dsp/granular_processor.cc
    libs/eurorack/clouds/dsp/correlator.cc
    libs/eurorack/clouds/dsp/mu_law.cc
    libs/eurorack/clouds/dsp/pvoc/frame_transformation.cc
    libs/eurorack/clouds/dsp/pvoc/phase_vocoder.cc
    libs/eurorack/clouds/dsp/pvoc/stft.cc
    libs/eurorack/clouds/resources.cc
    libs/stmlib/dsp/atan.cc
    libs/stmlib/dsp/units.cc
    libs/stmlib/utils/random.cc
)

# --- Plugin target ---

juce_add_plugin(CloudsVST
    COMPANY_NAME "K5SANO"
    PLUGIN_MANUFACTURER_CODE Clds
    PLUGIN_CODE Cld1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "CloudsVST"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(CloudsVST
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/CloudsEngine.cpp
        Source/SampleRateAdapter.cpp
        ${CLOUDS_DSP_SOURCES}
)

# Include paths:
# - Source/stubs/ — provides clouds/drivers/debug_pin.h stub (searched FIRST)
# - libs/eurorack/ — provides clouds/dsp/*, clouds/resources.h
# - libs/ — provides stmlib/stmlib.h via "stmlib/stmlib.h" includes

target_include_directories(CloudsVST
    PRIVATE
        Source/stubs
        Source
        libs/eurorack
        libs
)

target_compile_definitions(CloudsVST
    PRIVATE
        TEST=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(CloudsVST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)

# --- Tests (optional) ---

option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()

    juce_add_console_app(CloudsVSTTests
        PRODUCT_NAME "CloudsVSTTests"
    )

    target_sources(CloudsVSTTests
        PRIVATE
            Tests/PluginTests.cpp
            Tests/AudioProcessingTests.cpp
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp
            Source/CloudsEngine.cpp
            Source/SampleRateAdapter.cpp
            ${CLOUDS_DSP_SOURCES}
    )

    target_include_directories(CloudsVSTTests
        PRIVATE
            Source/stubs
            Source
            libs/eurorack
            libs
    )

    target_compile_definitions(CloudsVSTTests
        PRIVATE
            TEST=1
    )

    target_link_libraries(CloudsVSTTests
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
    )

    add_test(NAME CloudsVSTTests COMMAND CloudsVSTTests)
endif()
