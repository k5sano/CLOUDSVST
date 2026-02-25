#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include <signal.h>
#include <unistd.h>
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

static volatile bool keep_running = true;

void alarm_handler(int sig) {
    keep_running = false;
}

int main()
{
    std::cout << "=== Debug Stretch Test ===" << std::endl;

    // Set alarm to detect infinite loops
    signal(SIGALRM, alarm_handler);

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
    std::cout << "Step 1: Granular mode..." << std::endl;
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

    // Fill buffer
    std::cout << "Filling buffer..." << std::endl;
    for (int block = 0; block < 500 && keep_running; ++block)
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
    }
    if (!keep_running) {
        std::cout << "TIMEOUT during Granular mode!" << std::endl;
        return 1;
    }
    std::cout << "Buffer filled." << std::endl;

    // Now switch to Stretch mode
    std::cout << "\nStep 2: Switching to Stretch mode..." << std::endl;
    processor.set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);
    p->dry_wet = 1.0f;
    p->density = 0.7f;
    p->size = 0.5f;

    // Process one block with timeout
    std::cout << "Processing first Stretch block (with 5 second timeout)..." << std::endl;
    alarm(5);  // 5 second timeout

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

    std::cout << "Calling Prepare()..." << std::endl;
    processor.Prepare();
    std::cout << "Prepare done. Calling Process()..." << std::endl;
    processor.Process(inputFrames, outputFrames, kBlockSize);
    std::cout << "Process done!" << std::endl;

    alarm(0);  // Cancel alarm

    float maxOut = 0.0f;
    for (int i = 0; i < kBlockSize; ++i)
    {
        float a = std::abs(static_cast<float>(outputFrames[i].l));
        if (a > maxOut) maxOut = a;
    }
    std::cout << "First block maxOutput: " << maxOut << std::endl;

    if (!keep_running) {
        std::cout << "TIMEOUT: Process() appears to hang in Stretch mode!" << std::endl;
        return 1;
    }

    std::cout << "Test complete!" << std::endl;
    return 0;
}
