#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

int main()
{
    std::cout << "=== Buffered Stretch Test ===" << std::endl;

    const size_t kLargeBufferSize = 118784;
    const size_t kSmallBufferSize = 65535;
    auto largeBuffer = std::make_unique<uint8_t[]>(kLargeBufferSize);
    auto smallBuffer = std::make_unique<uint8_t[]>(kSmallBufferSize);
    std::memset(largeBuffer.get(), 0, kLargeBufferSize);
    std::memset(smallBuffer.get(), 0, kSmallBufferSize);

    clouds::GranularProcessor processor;
    processor.Init(largeBuffer.get(), kLargeBufferSize,
                   smallBuffer.get(), kSmallBufferSize);

    // Start with Granular to fill buffer
    std::cout << "Step 1: Granular mode to fill buffer..." << std::endl;
    processor.set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    processor.set_quality(0);
    processor.set_bypass(false);
    processor.set_silence(false);

    auto* p = processor.mutable_parameters();
    p->position = 0.5f; p->size = 0.5f; p->pitch = 0.0f;
    p->density = 0.7f; p->texture = 0.5f;
    p->dry_wet = 1.0f; p->stereo_spread = 0.0f;
    p->feedback = 0.0f; p->reverb = 0.0f;
    p->freeze = false; p->trigger = false; p->gate = false;

    for (int i = 0; i < 100; ++i)
        processor.Prepare();

    const int kBlockSize = 32;
    float phase = 0.0f;

    // Fill buffer with 1000 blocks (about 1 second)
    std::cout << "Processing 1000 blocks in Granular mode..." << std::endl;
    for (int block = 0; block < 1000; ++block)
    {
        float inL[kBlockSize], inR[kBlockSize];
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = std::sin(2.0f * 3.14159265f * phase);
            inL[i] = val * 0.5f;
            inR[i] = val * 0.5f;
            phase += 440.0f / 32000.0f;
        }

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

        if (block % 200 == 0)
        {
            float maxOut = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
            {
                float a = std::abs(static_cast<float>(outputFrames[i].l));
                if (a > maxOut) maxOut = a;
            }
            std::cout << "  Granular block " << block << " maxOut: " << maxOut << std::endl;
        }
    }

    // Now switch to Stretch mode
    std::cout << "\nStep 2: Switching to Stretch mode..." << std::endl;
    processor.set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);
    p->dry_wet = 1.0f;
    p->density = 0.7f;
    p->size = 0.5f;

    float maxOutputStretch = 0.0f;

    // Process 100 blocks in Stretch mode
    std::cout << "Processing 100 blocks in Stretch mode..." << std::endl;
    for (int block = 0; block < 100; ++block)
    {
        float inL[kBlockSize], inR[kBlockSize];
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = std::sin(2.0f * 3.14159265f * phase);
            inL[i] = val * 0.5f;
            inR[i] = val * 0.5f;
            phase += 440.0f / 32000.0f;
        }

        clouds::ShortFrame inputFrames[kBlockSize];
        clouds::ShortFrame outputFrames[kBlockSize];

        for (int i = 0; i < kBlockSize; ++i)
        {
            inputFrames[i].l = static_cast<int16_t>(inL[i] * 32767.0f);
            inputFrames[i].r = static_cast<int16_t>(inR[i] * 32767.0f);
        }
        std::memset(outputFrames, 0, sizeof(outputFrames));

        // Prepare 1回 → Process → Prepare 63回
        processor.Prepare();
        processor.Process(inputFrames, outputFrames, kBlockSize);
        for (int pp = 1; pp < 64; ++pp)
            processor.Prepare();

        for (int i = 0; i < kBlockSize; ++i)
        {
            float a = std::abs(static_cast<float>(outputFrames[i].l));
            if (a > maxOutputStretch) maxOutputStretch = a;
        }

        if (block % 20 == 0)
        {
            float blockMax = 0.0f;
            for (int i = 0; i < kBlockSize; ++i)
            {
                float a = std::abs(static_cast<float>(outputFrames[i].l));
                if (a > blockMax) blockMax = a;
            }
            std::cout << "  Stretch block " << block << " maxOut: " << blockMax << std::endl;
        }
    }

    std::cout << "\nFinal maxOutput (Stretch): " << maxOutputStretch << std::endl;

    if (maxOutputStretch < 1.0f)
        std::cout << "RESULT: Stretch mode produces NO output even after filling buffer" << std::endl;
    else
        std::cout << "RESULT: Stretch mode works when buffer is pre-filled!" << std::endl;

    return 0;
}
