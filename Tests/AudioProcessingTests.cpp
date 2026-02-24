#include <juce\_audio\_processors/juce\_audio\_processors.h> #include "PluginProcessor.h" #include "CloudsEngine.h"

//============================================================================== class AudioProcessingTests : public juce::UnitTest { public: AudioProcessingTests() : juce::UnitTest("Audio Processing Tests") {}

```
void runTest() override
{
    beginTest("CloudsEngine initialisation");
    {
        CloudsEngine engine;
        engine.init();
        // If we got here without crashing, init succeeded.
        expect(true, "Engine initialised without crash");
    }

    beginTest("CloudsEngine processes silence correctly");
    {
        CloudsEngine engine;
        engine.init();
        engine.setDryWet(0.0f);  // Full dry

        const int numSamples = 32;
        float inL[32] = {};
        float inR[32] = {};
        float outL[32] = {};
        float outR[32] = {};

        engine.process(inL, inR, outL, outR, numSamples);

        // With dry/wet = 0 (full dry) and silent input, output should be silent
        float maxAbs = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            maxAbs = std::max(maxAbs, std::abs(outL[i]));
            maxAbs = std::max(maxAbs, std::abs(outR[i]));
        }
        expect(maxAbs < 0.01f,
               "Silent input + full dry should produce near-silent output, got max="
               + juce::String(maxAbs));
    }

    beginTest("CloudsEngine processes multiple blocks without crash");
    {
        CloudsEngine engine;
        engine.init();

        const int numSamples = 32;
        float inL[32], inR[32], outL[32], outR[32];

        // Process 100 blocks (simulating ~0.1s at 32kHz)
        for (int block = 0; block < 100; ++block)
        {
            // Generate a simple sine wave input
            for (int i = 0; i < numSamples; ++i)
            {
                float t = static_cast<float>(block * numSamples + i) / 32000.0f;
                inL[i] = std::sin(2.0f * 3.14159f * 440.0f * t) * 0.5f;
                inR[i] = inL[i];
            }
            engine.process(inL, inR, outL, outR, numSamples);
        }
        expect(true, "Processed 100 blocks without crash");
    }

    beginTest("CloudsEngine mode switching");
    {
        CloudsEngine engine;
        engine.init();

        const int numSamples = 32;
        float inL[32] = {}, inR[32] = {}, outL[32], outR[32];

        for (int mode = 0; mode < 4; ++mode)
        {
            engine.setPlaybackMode(mode);
            // Process several blocks to let mode transition complete
            for (int b = 0; b < 10; ++b)
                engine.process(inL, inR, outL, outR, numSamples);
        }
        expect(true, "All 4 modes processed without crash");
    }

    beginTest("CloudsEngine quality switching");
    {
        CloudsEngine engine;
        engine.init();

        const int numSamples = 32;
        float inL[32] = {}, inR[32] = {}, outL[32], outR[32];

        for (int q = 0; q < 4; ++q)
        {
            engine.setQuality(q);
            for (int b = 0; b < 10; ++b)
                engine.process(inL, inR, outL, outR, numSamples);
        }
        expect(true, "All 4 quality settings processed without crash");
    }

    beginTest("Processor prepareToPlay at various sample rates");
    {
        const double sampleRates[] = { 44100.0, 48000.0, 96000.0 };
        const int blockSizes[] = { 64, 128, 256, 512, 1024 };

        for (auto sr : sampleRates)
        {
            for (auto bs : blockSizes)
            {
                CloudsVSTProcessor proc;
                proc.prepareToPlay(sr, bs);

                juce::AudioBuffer<float> buffer(2, bs);
                buffer.clear();
                juce::MidiBuffer midi;

                // Process a few blocks
                for (int i = 0; i < 5; ++i)
                    proc.processBlock(buffer, midi);

                expect(true, "SR=" + juce::String(sr) + " BS=" + juce::String(bs) + " OK");
            }
        }
    }

    beginTest("Freeze behaviour");
    {
        CloudsEngine engine;
        engine.init();
        engine.setFreeze(true);
        engine.setDryWet(1.0f);

        const int numSamples = 32;
        float inL[32] = {}, inR[32] = {}, outL[32], outR[32];

        // With freeze=true and no prior audio, output should still be near-silent
        for (int b = 0; b < 10; ++b)
            engine.process(inL, inR, outL, outR, numSamples);

        expect(true, "Freeze mode processed without crash");
    }
}
```

};

static AudioProcessingTests audioProcessingTests;