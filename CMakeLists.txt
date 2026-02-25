```cmake
Copycmake_minimum_required(VERSION 3.22)
project(MetalCosmos VERSION 1.0.0)

find_package(JUCE CONFIG REQUIRED)

juce_add_plugin(MetalCosmos
    PLUGIN_MANUFACTURER_CODE K5sn
    PLUGIN_CODE Mt2p
    FORMATS VST3 Standalone
    PRODUCT_NAME "MetalCosmos"
)

juce_generate_juce_header(MetalCosmos)

target_sources(MetalCosmos PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/DiodeFeedbackClipper.cpp
    Source/DSP/DiodeMorpher.cpp
    Source/DSP/OnePoleFilter.cpp
    Source/DSP/BiquadFilter.cpp
    Source/DSP/MT2GainStage.cpp
    Source/DSP/MT2ToneStack.cpp
)

target_include_directories(MetalCosmos PRIVATE
    Source
    Source/DSP
    scaffold
    scaffold/DSP
)

target_compile_features(MetalCosmos PRIVATE cxx_std_17)

if(MSVC)
    target_compile_definitions(MetalCosmos PRIVATE _USE_MATH_DEFINES)
endif()

target_link_libraries(MetalCosmos
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_audio_plugin_client
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
)

# --- Tests ---
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()

    find_package(Catch2 3 QUIET)
    if(Catch2_FOUND)
        add_executable(MetalCosmosTests
            tests/TestMain.cpp
            tests/DiodeClipperTest.cpp
            tests/DiodeMorpherTest.cpp
            tests/ToneStackTest.cpp
            tests/ParameterTest.cpp
            Source/DSP/DiodeFeedbackClipper.cpp
            Source/DSP/DiodeMorpher.cpp
            Source/DSP/OnePoleFilter.cpp
            Source/DSP/BiquadFilter.cpp
            Source/DSP/MT2GainStage.cpp
            Source/DSP/MT2ToneStack.cpp
        )
        target_include_directories(MetalCosmosTests PRIVATE
            Source
            Source/DSP
            scaffold
            scaffold/DSP
        )
        target_compile_features(MetalCosmosTests PRIVATE cxx_std_17)
        target_link_libraries(MetalCosmosTests PRIVATE
            Catch2::Catch2WithMain
            juce::juce_audio_basics
            juce::juce_audio_processors
            juce::juce_dsp
        )
        if(MSVC)
            target_compile_definitions(MetalCosmosTests PRIVATE _USE_MATH_DEFINES)
        endif()

        include(Catch2::Catch)
        catch_discover_tests(MetalCosmosTests)
    endif()
endif()
Copy
```