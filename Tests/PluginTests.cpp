#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "Parameters.h"

//==============================================================================
// Basic parameter validation tests
//==============================================================================
class ParameterTests : public juce::UnitTest
{
public:
    ParameterTests() : juce::UnitTest("Parameter Tests") {}

    void runTest() override
    {
        beginTest("Parameter layout creation");
        {
            // ParameterLayout creation is tested implicitly in Processor construction test
            // We'll verify all parameters exist there
        }

        beginTest("Processor construction");
        {
            CloudsVSTProcessor proc;
            auto& apvts = proc.getAPVTS();

            // Verify all parameters exist
            expect(apvts.getParameter("position") != nullptr);
            expect(apvts.getParameter("size") != nullptr);
            expect(apvts.getParameter("pitch") != nullptr);
            expect(apvts.getParameter("density") != nullptr);
            expect(apvts.getParameter("texture") != nullptr);
            expect(apvts.getParameter("dry_wet") != nullptr);
            expect(apvts.getParameter("stereo_spread") != nullptr);
            expect(apvts.getParameter("feedback") != nullptr);
            expect(apvts.getParameter("reverb") != nullptr);
            expect(apvts.getParameter("freeze") != nullptr);
            expect(apvts.getParameter("trigger") != nullptr);
            expect(apvts.getParameter("playback_mode") != nullptr);
            expect(apvts.getParameter("quality") != nullptr);
            expect(apvts.getParameter("input_gain") != nullptr);
        }

        beginTest("Default parameter values");
        {
            CloudsVSTProcessor proc;
            auto& apvts = proc.getAPVTS();

            auto checkDefault = [&](const juce::String& id, float expected, float tolerance = 0.01f)
            {
                auto* param = apvts.getRawParameterValue(id);
                expectWithinAbsoluteError(param->load(), expected, tolerance,
                    id + " default value mismatch");
            };

            checkDefault("position", 0.5f);
            checkDefault("size", 0.5f);
            checkDefault("pitch", 0.0f);
            checkDefault("density", 0.5f);
            checkDefault("texture", 0.5f);
            checkDefault("dry_wet", 0.5f);
            checkDefault("stereo_spread", 0.0f);
            checkDefault("feedback", 0.0f);
            checkDefault("reverb", 0.0f);
            checkDefault("input_gain", 0.0f);
        }

        beginTest("State save and restore");
        {
            juce::MemoryBlock stateData;

            {
                CloudsVSTProcessor proc;
                auto& apvts = proc.getAPVTS();
                apvts.getParameter("position")->setValueNotifyingHost(0.75f);
                apvts.getParameter("feedback")->setValueNotifyingHost(0.6f);
                proc.getStateInformation(stateData);
            }

            {
                CloudsVSTProcessor proc;
                proc.setStateInformation(stateData.getData(),
                                          static_cast<int>(stateData.getSize()));
                auto& apvts = proc.getAPVTS();

                // Values should be restored
                auto* posParam = apvts.getRawParameterValue("position");
                expectWithinAbsoluteError(posParam->load(), 0.75f, 0.02f,
                    "Position not restored correctly");
            }
        }
    }
};

static ParameterTests parameterTests;

//==============================================================================
// Entry point for juce console test runner
//==============================================================================
int main(int /*argc*/, char* /*argv*/[])
{
    juce::UnitTestRunner runner;
    runner.runAllTests();

    int numFailures = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        if (auto* result = runner.getResult(i))
            numFailures += result->failures;
    }

    return numFailures > 0 ? 1 : 0;
}
