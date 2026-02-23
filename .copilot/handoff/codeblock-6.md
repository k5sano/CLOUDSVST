Copy
cmake_minimum_required(VERSION 3.22)

project(CloudsVST VERSION 1.0.0)

# JUCE を add_subdirectory で取り込む（JUCEをサブモジュールとして配置済み前提）
# もしくは find_package(JUCE) でも可
add_subdirectory(JUCE)

# ---- Mutable Instruments Clouds DSP ソースの収集 ----
set(MI_EURORACK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/eurorack)

set(CLOUDS_DSP_SOURCES
    ${MI_EURORACK_DIR}/clouds/dsp/correlator.cc
    ${MI_EURORACK_DIR}/clouds/dsp/granular_processor.cc
    ${MI_EURORACK_DIR}/clouds/dsp/granular_sample_player.cc
    ${MI_EURORACK_DIR}/clouds/dsp/looping_sample_player.cc
    ${MI_EURORACK_DIR}/clouds/dsp/mu_law.cc
    ${MI_EURORACK_DIR}/clouds/dsp/pvoc/frame_transformation.cc
    ${MI_EURORACK_DIR}/clouds/dsp/pvoc/phase_vocoder.cc
    ${MI_EURORACK_DIR}/clouds/dsp/pvoc/stft.cc
    ${MI_EURORACK_DIR}/clouds/dsp/sample_rate_converter.cc
    ${MI_EURORACK_DIR}/clouds/dsp/wsola_sample_player.cc
    ${MI_EURORACK_DIR}/clouds/resources.cc
)

# ---- JUCE Plugin ターゲット ----
juce_add_plugin(CloudsVST
    COMPANY_NAME "MI_JUCE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE MiJc
    PLUGIN_CODE Clds
    FORMATS VST3 Standalone
    PRODUCT_NAME "Clouds"
)

target_sources(CloudsVST
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        ${CLOUDS_DSP_SOURCES}
)

target_include_directories(CloudsVST
    PRIVATE
        ${MI_EURORACK_DIR}
)

# TEST マクロを定義して ARM アセンブリを回避
target_compile_definitions(CloudsVST
    PUBLIC
        TEST
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(CloudsVST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

Copy