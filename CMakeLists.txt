cmake\_minimum\_required(VERSION 3.22) project(CloudsVST VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE\_CXX\_STANDARD 17) set(CMAKE\_CXX\_STANDARD\_REQUIRED ON)

# \--- Fetch JUCE ---

include(FetchContent) FetchContent\_Declare(JUCE GIT\_REPOSITORY [https://github.com/juce-framework/JUCE.git](https://github.com/juce-framework/JUCE.git) GIT\_TAG 8.0.4 ) FetchContent\_MakeAvailable(JUCE)

# \--- Clouds DSP sources from eurorack submodule ---

set(CLOUDS\_DSP\_SOURCES libs/eurorack/clouds/dsp/granular\_processor.cc libs/eurorack/clouds/dsp/correlator.cc libs/eurorack/clouds/dsp/mu\_law.cc libs/eurorack/clouds/dsp/pvoc/frame\_transformation.cc libs/eurorack/clouds/dsp/pvoc/phase\_vocoder.cc libs/eurorack/clouds/dsp/pvoc/stacking\_order.cc libs/eurorack/clouds/resources.cc )

# \--- Plugin target ---

juce\_add\_plugin(CloudsVST COMPANY\_NAME "CloudsVST" PLUGIN\_MANUFACTURER\_CODE Clds PLUGIN\_CODE Cld1 FORMATS VST3 AU Standalone PRODUCT\_NAME "CloudsVST" IS\_SYNTH FALSE NEEDS\_MIDI\_INPUT FALSE NEEDS\_MIDI\_OUTPUT FALSE IS\_MIDI\_EFFECT FALSE EDITOR\_WANTS\_KEYBOARD\_FOCUS FALSE COPY\_PLUGIN\_AFTER\_BUILD TRUE )

target\_sources(CloudsVST PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/CloudsEngine.cpp Source/SampleRateAdapter.cpp ${CLOUDS\_DSP\_SOURCES} )

# Include paths:

# Source/stubs/ — provides clouds/drivers/debug\_pin.h stub (searched FIRST)

# libs/eurorack/ — provides clouds/dsp/\*, clouds/resources.h

# libs/ — provides stmlib/stmlib.h via "stmlib/stmlib.h" includes

target\_include\_directories(CloudsVST PRIVATE Source/stubs Source libs/eurorack libs )

target\_compile\_definitions(CloudsVST PRIVATE TEST=1 JUCE\_VST3\_CAN\_REPLACE\_VST2=0 JUCE\_WEB\_BROWSER=0 JUCE\_USE\_CURL=0 PUBLIC JUCE\_DISPLAY\_SPLASH\_SCREEN=0 )

target\_link\_libraries(CloudsVST PRIVATE juce::juce\_audio\_utils juce::juce\_dsp PUBLIC juce::juce\_recommended\_config\_flags juce::juce\_recommended\_warning\_flags )

# \--- Tests (optional) ---

option(BUILD\_TESTS "Build unit tests" ON)

if(BUILD\_TESTS) enable\_testing()

```
juce_add_console_app(CloudsVSTTests PRODUCT_NAME "CloudsVSTTests")

target_sources(CloudsVSTTests
    PRIVATE
        Tests/PluginTests.cpp
        Tests/AudioProcessingTests.cpp
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
```

endif()