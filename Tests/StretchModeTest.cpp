#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

// 440Hz サイン波を生成
void generateSine(float* bufL, float* bufR, int numSamples, float freq, float sampleRate, float& phase)
{
    for (int i = 0; i < numSamples; ++i)
    {
        float val = std::sin(2.0f * 3.14159265f * phase);
        bufL[i] = val * 0.5f;
        bufR[i] = val * 0.5f;
        phase += freq / sampleRate;
        if (phase >= 1.0f) phase -= 1.0f;
    }
}

int main()
{
    std::cout << "=== Stretch Mode Debug Test ===" << std::endl;

    // バッファ確保
    const size_t kLargeBufferSize = 118784;
    const size_t kSmallBufferSize = 65535;
    auto largeBuffer = std::make_unique<uint8_t[]>(kLargeBufferSize);
    auto smallBuffer = std::make_unique<uint8_t[]>(kSmallBufferSize);
    std::memset(largeBuffer.get(), 0, kLargeBufferSize);
    std::memset(smallBuffer.get(), 0, kSmallBufferSize);

    clouds::GranularProcessor processor;
    processor.Init(largeBuffer.get(), kLargeBufferSize,
                   smallBuffer.get(), kSmallBufferSize);

    // === Test A: Granular モードで正常動作確認 ===
    std::cout << "\n--- Test A: Granular mode baseline ---" << std::endl;
    processor.set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    processor.set_quality(0);
    processor.set_bypass(false);
    processor.set_silence(false);

    auto* p = processor.mutable_parameters();
    p->position = 0.5f; p->size = 0.5f; p->pitch = 0.0f;
    p->density = 0.7f; p->texture = 0.5f;
    p->dry_wet = 1.0f; p->stereo_spread = 0.0f;
    p->feedback = 0.0f; p->reverb = 0.3f;
    p->freeze = false; p->trigger = false; p->gate = false;

    // Prepare を複数回呼ぶ（初期化）
    for (int i = 0; i < 100; ++i)
        processor.Prepare();

    float phase = 0.0f;
    const int kBlockSize = 32;
    float maxOutputGranular = 0.0f;

    // 200ブロック処理（短縮）
    for (int block = 0; block < 200; ++block)
    {
        float inL[kBlockSize], inR[kBlockSize];
        generateSine(inL, inR, kBlockSize, 440.0f, 32000.0f, phase);

        clouds::ShortFrame inputFrames[kBlockSize];
        clouds::ShortFrame outputFrames[kBlockSize];

        for (int i = 0; i < kBlockSize; ++i)
        {
            inputFrames[i].l = static_cast<int16_t>(inL[i] * 32767.0f);
            inputFrames[i].r = static_cast<int16_t>(inR[i] * 32767.0f);
        }
        std::memset(outputFrames, 0, sizeof(outputFrames));

        processor.Prepare();
        processor.Process(inputFrames, outputFrames, kBlockSize);

        for (int i = 0; i < kBlockSize; ++i)
        {
            float absVal = std::abs(static_cast<float>(outputFrames[i].l));
            if (absVal > maxOutputGranular) maxOutputGranular = absVal;
        }

        if (block % 50 == 0)
        {
            float blockMax = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
            {
                float a = std::abs(static_cast<float>(outputFrames[i].l));
                if (a > blockMax) blockMax = a;
            }
            std::cout << "  Granular block " << block << ": outputMax=" << blockMax << std::endl;
        }
    }
    std::cout << "  Granular total maxOutput: " << maxOutputGranular << std::endl;

    // === Test B: Stretch モードに切り替え ===
    std::cout << "\n--- Test B: Switch to Stretch mode ---" << std::endl;
    processor.set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);
    p->dry_wet = 1.0f;
    p->density = 0.7f;
    p->size = 0.5f;

    float maxOutputStretch = 0.0f;

    // 200ブロック処理（短縮）
    for (int block = 0; block < 200; ++block)
    {
        float inL[kBlockSize], inR[kBlockSize];
        generateSine(inL, inR, kBlockSize, 440.0f, 32000.0f, phase);

        clouds::ShortFrame inputFrames[kBlockSize];
        clouds::ShortFrame outputFrames[kBlockSize];

        for (int i = 0; i < kBlockSize; ++i)
        {
            inputFrames[i].l = static_cast<int16_t>(inL[i] * 32767.0f);
            inputFrames[i].r = static_cast<int16_t>(inR[i] * 32767.0f);
        }
        std::memset(outputFrames, 0, sizeof(outputFrames));

        // Prepare 1回 → Process → Prepare 255回
        processor.Prepare();
        processor.Process(inputFrames, outputFrames, kBlockSize);
        for (int pp = 1; pp < 256; ++pp)
            processor.Prepare();

        for (int i = 0; i < kBlockSize; ++i)
        {
            float absVal = std::abs(static_cast<float>(outputFrames[i].l));
            if (absVal > maxOutputStretch) maxOutputStretch = absVal;
        }

        if (block % 50 == 0)
        {
            float blockMax = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
            {
                float a = std::abs(static_cast<float>(outputFrames[i].l));
                if (a > blockMax) blockMax = a;
            }
            std::cout << "  Stretch block " << block << ": outputMax=" << blockMax << std::endl;
        }
    }
    std::cout << "  Stretch total maxOutput: " << maxOutputStretch << std::endl;

    // === Test C: Stretch で全Prepare を Process の前に呼ぶ（比較） ===
    std::cout << "\n--- Test C: Stretch with all Prepare BEFORE Process ---" << std::endl;
    // 新しいprocessorインスタンスで再テスト
    std::memset(largeBuffer.get(), 0, kLargeBufferSize);
    std::memset(smallBuffer.get(), 0, kSmallBufferSize);
    clouds::GranularProcessor processor2;
    processor2.Init(largeBuffer.get(), kLargeBufferSize,
                    smallBuffer.get(), kSmallBufferSize);

    processor2.set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    processor2.set_quality(0);
    processor2.set_bypass(false);
    processor2.set_silence(false);

    auto* p2 = processor2.mutable_parameters();
    p2->position = 0.5f; p2->size = 0.5f; p2->pitch = 0.0f;
    p2->density = 0.7f; p2->texture = 0.5f;
    p2->dry_wet = 1.0f; p2->stereo_spread = 0.0f;
    p2->feedback = 0.0f; p2->reverb = 0.3f;
    p2->freeze = false; p2->trigger = false; p2->gate = false;

    for (int i = 0; i < 100; ++i) processor2.Prepare();

    // バッファを埋める
    phase = 0.0f;
    for (int block = 0; block < 200; ++block)
    {
        float inL[kBlockSize], inR[kBlockSize];
        generateSine(inL, inR, kBlockSize, 440.0f, 32000.0f, phase);
        clouds::ShortFrame inf[kBlockSize], outf[kBlockSize];
        for (int i = 0; i < kBlockSize; ++i) {
            inf[i].l = static_cast<int16_t>(inL[i] * 32767.0f);
            inf[i].r = static_cast<int16_t>(inR[i] * 32767.0f);
        }
        std::memset(outf, 0, sizeof(outf));
        processor2.Prepare();
        processor2.Process(inf, outf, kBlockSize);
    }

    // Stretch に切り替え
    processor2.set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);
    float maxOutputStretchBefore = 0.0f;
    for (int block = 0; block < 200; ++block)
    {
        float inL[kBlockSize], inR[kBlockSize];
        generateSine(inL, inR, kBlockSize, 440.0f, 32000.0f, phase);
        clouds::ShortFrame inf[kBlockSize], outf[kBlockSize];
        for (int i = 0; i < kBlockSize; ++i) {
            inf[i].l = static_cast<int16_t>(inL[i] * 32767.0f);
            inf[i].r = static_cast<int16_t>(inR[i] * 32767.0f);
        }
        std::memset(outf, 0, sizeof(outf));
        // 全部 Process の前
        for (int pp = 0; pp < 256; ++pp)
            processor2.Prepare();
        processor2.Process(inf, outf, kBlockSize);

        for (int i = 0; i < kBlockSize; ++i)
        {
            float a = std::abs(static_cast<float>(outf[i].l));
            if (a > maxOutputStretchBefore) maxOutputStretchBefore = a;
        }
        if (block % 50 == 0)
        {
            float blockMax = 0.0f;
            for (int i = 0; i < kBlockSize; ++i) {
                float a = std::abs(static_cast<float>(outf[i].l));
                if (a > blockMax) blockMax = a;
            }
            std::cout << "  Stretch(before) block " << block << ": outputMax=" << blockMax << std::endl;
        }
    }
    std::cout << "  Stretch(before) total maxOutput: " << maxOutputStretchBefore << std::endl;

    // === 結果サマリ ===
    std::cout << "\n=== SUMMARY ===" << std::endl;
    std::cout << "Granular maxOutput:        " << maxOutputGranular << std::endl;
    std::cout << "Stretch(after) maxOutput:  " << maxOutputStretch << std::endl;
    std::cout << "Stretch(before) maxOutput: " << maxOutputStretchBefore << std::endl;

    if (maxOutputStretch < 1.0f && maxOutputStretchBefore < 1.0f)
        std::cout << "RESULT: Both Stretch variants produce NO output - issue is NOT Prepare ordering" << std::endl;
    else if (maxOutputStretch > 1.0f && maxOutputStretchBefore < 1.0f)
        std::cout << "RESULT: Prepare AFTER Process works - current fix is correct" << std::endl;
    else if (maxOutputStretch < 1.0f && maxOutputStretchBefore > 1.0f)
        std::cout << "RESULT: Prepare BEFORE Process works - need to revert fix" << std::endl;
    else
        std::cout << "RESULT: Both variants produce output" << std::endl;

    return 0;
}
