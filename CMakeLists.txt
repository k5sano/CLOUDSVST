cmake_minimum_required(VERSION 3.22)
project(CloudsCOSMOS VERSION 1.0.0 LANGUAGES C CXX)

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

juce_add_plugin(CloudsCOSMOS
    COMPANY_NAME "K5SANO"
    COMPANY_website "https://github.com/k5sano"
    COMPANY_email "contact@k5sano.com"
    PLUGIN_MANUFACTURER_CODE Clds
    PLUGIN_CODE Cld1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "CloudsCOSMOS"
    PLUGIN_DESCRIPTION "Granular Texture Synthesis"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(CloudsCOSMOS
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

target_include_directories(CloudsCOSMOS
    PRIVATE
        Source/stubs
        Source
        libs/eurorack
        libs
)

target_compile_definitions(CloudsCOSMOS
    PRIVATE
        TEST=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
    PUBLIC
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(CloudsCOSMOS
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

    juce_add_console_app(CloudsCOSMOSTests
        PRODUCT_NAME "CloudsCOSMOSTests"
    )

    target_sources(CloudsCOSMOSTests
        PRIVATE
            Tests/PluginTests.cpp
            Tests/AudioProcessingTests.cpp
            Source/PluginProcessor.cpp
            Source/PluginEditor.cpp
            Source/CloudsEngine.cpp
            Source/SampleRateAdapter.cpp
            ${CLOUDS_DSP_SOURCES}
    )

    target_include_directories(CloudsCOSMOSTests
        PRIVATE
            Source/stubs
            Source
            libs/eurorack
            libs
    )

    target_compile_definitions(CloudsCOSMOSTests
        PRIVATE
            TEST=1
    )

    target_link_libraries(CloudsCOSMOSTests
        PRIVATE
            juce::juce_audio_utils
            juce::juce_dsp
    )

    add_test(NAME CloudsCOSMOSTests COMMAND CloudsCOSMOSTests)
endif()
