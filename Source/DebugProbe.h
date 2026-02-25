#pragma once

#include <juce_core/juce_core.h>
#include <cmath>
#include <fstream>
#include <mutex>

class DebugProbe
{
public:
    DebugProbe(const char* name) : name_(name) {}

    void measureStereo(const float* L, const float* R, int numSamples,
                       int printInterval = 200)
    {
        if (numSamples <= 0) return;

        float sum = 0.0f;
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float v = (std::abs(L[i]) + std::abs(R[i])) * 0.5f;
            sum += v * v;
            if (v > peak) peak = v;
        }
        lastRms_ = std::sqrt(sum / static_cast<float>(numSamples));
        lastPeak_ = peak;

        if (++blockCount_ >= printInterval)
        {
            blockCount_ = 0;
            juce::String msg = juce::String("[PROBE ") + name_ + "] "
                + "RMS=" + juce::String(lastRms_, 6)
                + "  Peak=" + juce::String(lastPeak_, 6)
                + "  (" + (lastPeak_ > 0.0001f ? "SIGNAL" : "SILENT") + ")";

            // JUCE の DBG はデバッグ出力に送る（IDE のコンソール等）
            DBG(msg);

            // ファイルにも書き出す（確実にログが残る）
            writeToFile(msg);
        }
    }

    float lastRms() const { return lastRms_; }
    float lastPeak() const { return lastPeak_; }

private:
    void writeToFile(const juce::String& msg)
    {
        static std::once_flag flag;
        static juce::File logFile;
        static std::mutex logMutex;
        std::call_once(flag, [&]()
        {
            logFile = juce::File::getSpecialLocation(
                juce::File::userDesktopDirectory).getChildFile("CloudsVST_debug.log");
            // ファイルを空にする
            logFile.replaceWithText("=== CloudsVST Debug Log ===\n");
        });
        std::lock_guard<std::mutex> lock(logMutex);
        logFile.appendText(msg + "\n");
    }

    const char* name_;
    float lastRms_ = 0.0f;
    float lastPeak_ = 0.0f;
    int blockCount_ = 0;
};
