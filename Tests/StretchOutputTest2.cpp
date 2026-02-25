#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

int main()
{
    std::cout << "=== Stretch Output Debug Test ===" << std::endl;

    const size_t kLargeBufferSize = 118784;
    const size_t kSmallBufferSize = 65536 - 128;
    auto largeBuffer = std::make_unique<uint8_t[]>(kLargeBufferSize);
    auto smallBuffer = std::make_unique<uint8_t[]>(kSmallBufferSize);
    std::memset(largeBuffer.get(), 0, kLargeBufferSize);
    std::memset(smallBuffer.get(), 0, kSmallBufferSize);

    clouds::GranularProcessor processor;
    processor.Init(largeBuffer.get(), kLargeBufferSize,
                   smallBuffer.get(), kSmallBufferSize);

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

    const int kBlockSize = 32;

    // VCV Rack と同様: 空回しして previous_playback_mode_ を安定
    std::cout << "Stabilizing with 10 dummy blocks..." << std::endl;
    {
        clouds::ShortFrame dummyIn[kBlockSize] = {};
        clouds::ShortFrame dummyOut[kBlockSize] = {};
        for (int i = 0; i < 10; ++i) {
            processor.Prepare();
            processor.Process(dummyIn, dummyOut, kBlockSize);
        }
    }

    float phase = 0.0f;

    // Step 1: Fill buffer with 500 blocks of sine wave in Granular
    std::cout << "\nStep 1: Filling buffer (500 blocks, Granular)..." << std::endl;
    for (int block = 0; block < 500; ++block)
    {
        clouds::ShortFrame input[kBlockSize], output[kBlockSize];
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = std::sin(2.0f * 3.14159265f * phase) * 0.5f;
            input[i].l = static_cast<int16_t>(val * 32767.0f);
            input[i].r = static_cast<int16_t>(val * 32767.0f);
            phase += 440.0f / 32000.0f;
        }
        std::memset(output, 0, sizeof(output));
        processor.Prepare();
        processor.Process(input, output, kBlockSize);

        if (block == 0 || block == 99 || block == 499) {
            float maxL = 0;
            for (int i = 0; i < kBlockSize; ++i) {
                float al = std::abs(static_cast<float>(output[i].l));
                if (al > maxL) maxL = al;
            }
            std::cout << "  Block " << block << " maxL=" << maxL << std::endl;
        }
    }
    std::cout << "Buffer filled." << std::endl;

    // Step 2: Switch to Stretch
    std::cout << "\nStep 2: Switch to Stretch mode" << std::endl;
    processor.set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);
    p->size = 0.5f;
    p->density = 0.7f;
    p->texture = 0.5f;
    p->dry_wet = 1.0f;
    p->pitch = 0.0f;
    p->freeze = false;

    // Step 3: Process 200 blocks in Stretch, print every 10
    std::cout << "Processing 200 blocks in Stretch mode..." << std::endl;
    for (int block = 0; block < 200; ++block)
    {
        clouds::ShortFrame input[kBlockSize], output[kBlockSize];
        for (int i = 0; i < kBlockSize; ++i)
        {
            float val = std::sin(2.0f * 3.14159265f * phase) * 0.5f;
            input[i].l = static_cast<int16_t>(val * 32767.0f);
            input[i].r = static_cast<int16_t>(val * 32767.0f);
            phase += 440.0f / 32000.0f;
        }
        std::memset(output, 0, sizeof(output));
        processor.Prepare();
        processor.Process(input, output, kBlockSize);

        if (block % 10 == 0)
        {
            float maxL = 0, maxR = 0;
            for (int i = 0; i < kBlockSize; ++i)
            {
                float al = std::abs(static_cast<float>(output[i].l));
                float ar = std::abs(static_cast<float>(output[i].r));
                if (al > maxL) maxL = al;
                if (ar > maxR) maxR = ar;
            }
            std::cout << "  Block " << block
                      << " maxL=" << maxL << " maxR=" << maxR << std::endl;
        }
    }

    // Step 4: Try with freeze=true
    std::cout << "\nStep 3: Freeze ON + Stretch..." << std::endl;
    p->freeze = true;
    for (int block = 0; block < 100; ++block)
    {
        clouds::ShortFrame input[kBlockSize], output[kBlockSize];
        std::memset(input, 0, sizeof(input));
        std::memset(output, 0, sizeof(output));
        processor.Prepare();
        processor.Process(input, output, kBlockSize);

        if (block % 10 == 0)
        {
            float maxL = 0;
            for (int i = 0; i < kBlockSize; ++i)
            {
                float al = std::abs(static_cast<float>(output[i].l));
                if (al > maxL) maxL = al;
            }
            std::cout << "  Freeze block " << block << " maxL=" << maxL << std::endl;
        }
    }

    std::cout << "\nDone!" << std::endl;
    return 0;
}
