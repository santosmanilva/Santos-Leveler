#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <vector>

#include "TruePeakCoefficients.h"
#include "Constants.h"
#include "DenormalProtection.h"

class SantosTruePeakLimiter
{
public:
    void prepare (double newSampleRate, int newNumChannels)
    {
        sampleRate = std::max (1.0, newSampleRate);
        numChannels = std::clamp (newNumChannels, 1, 2);

        using namespace SantosConstants;
        lookaheadSamples = std::max (1, static_cast<int> (std::round (sampleRate * (truePeakLookaheadMs * 0.001))));
        delayBufferSize = lookaheadSamples + 1;

        wetDelayL.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        wetDelayR.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        dryDelayL.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        dryDelayR.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);

        releaseAlpha = static_cast<float> (
            1.0 - std::exp (-1.0 / (truePeakReleaseMs * 0.001 * sampleRate)));

        reset();
    }

    void setCeilingDbTP (float dbTP) noexcept
    {
        using namespace SantosConstants;
        const auto clamped = std::clamp (dbTP, minCeilingDbTP, maxCeilingDbTP);
        ceilingDbTP.store (clamped, std::memory_order_release);
        ceilingLinear.store (std::pow (10.0f, clamped / 20.0f), std::memory_order_release);
    }

    float getCeilingDbTP() const noexcept { return ceilingDbTP.load (std::memory_order_acquire); }

    void reset() noexcept
    {
        for (auto& channel : detectorHistory)
            channel.fill (0.0f);

        std::fill (wetDelayL.begin(), wetDelayL.end(), 0.0f);
        std::fill (wetDelayR.begin(), wetDelayR.end(), 0.0f);
        std::fill (dryDelayL.begin(), dryDelayL.end(), 0.0f);
        std::fill (dryDelayR.begin(), dryDelayR.end(), 0.0f);

        detectorWriteIndex = 0;
        delayWriteIndex = 0;
        currentGain = 1.0f;
        holdSamplesRemaining = 0;
        latestTruePeakLinear = 0.0f;
    }

    void process (float wetL,
                  float wetR,
                  float dryL,
                  float dryR,
                  float& limitedWetL,
                  float& limitedWetR,
                  float& delayedDryL,
                  float& delayedDryR) noexcept
    {
        const auto truePeak = detectTruePeak (wetL, wetR);
        latestTruePeakLinear = denormalize(truePeak);

        const auto currentCeilingLinear = ceilingLinear.load (std::memory_order_acquire);
        const auto requiredGain = truePeak > currentCeilingLinear
            ? std::clamp (currentCeilingLinear / std::max (truePeak, 1.0e-9f), 0.0f, 1.0f)
            : 1.0f;

        // Instant attack is safe because the audio path is delayed by 1 ms.
        // Hold covers the lookahead plus the FIR group delay samples before release begins.
        if (requiredGain < currentGain)
            currentGain = requiredGain;

        if (requiredGain < 1.0f)
            holdSamplesRemaining = std::max (holdSamplesRemaining, lookaheadSamples + truePeakHoldExtraSamples);

        if (holdSamplesRemaining > 0)
        {
            --holdSamplesRemaining;
        }
        else if (currentGain < 1.0f)
        {
            currentGain += releaseAlpha * (1.0f - currentGain);
            if (currentGain > 0.999999f)
                currentGain = 1.0f;
        }

        wetDelayL[static_cast<std::size_t> (delayWriteIndex)] = wetL;
        wetDelayR[static_cast<std::size_t> (delayWriteIndex)] = wetR;
        dryDelayL[static_cast<std::size_t> (delayWriteIndex)] = dryL;
        dryDelayR[static_cast<std::size_t> (delayWriteIndex)] = dryR;

        auto readIndex = delayWriteIndex - lookaheadSamples;
        if (readIndex < 0)
            readIndex += delayBufferSize;

        limitedWetL = wetDelayL[static_cast<std::size_t> (readIndex)] * currentGain;
        limitedWetR = wetDelayR[static_cast<std::size_t> (readIndex)] * currentGain;
        delayedDryL = dryDelayL[static_cast<std::size_t> (readIndex)];
        delayedDryR = dryDelayR[static_cast<std::size_t> (readIndex)];

        if (++delayWriteIndex >= delayBufferSize)
            delayWriteIndex = 0;
    }

    int getLatencySamples() const noexcept { return lookaheadSamples; }

    float getGainReductionDb() const noexcept
    {
        if (currentGain <= 1.0e-8f)
            return -100.0f;
        return std::min (0.0f, 20.0f * std::log10 (currentGain));
    }

    float getDetectedTruePeakDbTP() const noexcept
    {
        if (latestTruePeakLinear <= 1.0e-8f)
            return -100.0f;
        return 20.0f * std::log10 (latestTruePeakLinear);
    }

private:
    float detectTruePeak (float left, float right) noexcept
    {
        detectorHistory[0][static_cast<std::size_t> (detectorWriteIndex)] = left;
        detectorHistory[1][static_cast<std::size_t> (detectorWriteIndex)] = right;

        float maximum = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int phase = 0; phase < 4; ++phase)
            {
                float value = 0.0f;
                auto historyIndex = detectorWriteIndex;

                for (int tap = 0; tap < 12; ++tap)
                {
                    value += detectorHistory[static_cast<std::size_t> (channel)]
                                            [static_cast<std::size_t> (historyIndex)]
                             * SantosTruePeakCoefficients::values[static_cast<std::size_t> (tap)]
                                           [static_cast<std::size_t> (phase)];

                    if (--historyIndex < 0)
                        historyIndex = 11;
                }

                maximum = std::max (maximum, std::abs (value));
            }
        }

        if (++detectorWriteIndex >= 12)
            detectorWriteIndex = 0;

        return maximum;
    }

    double sampleRate = 48000.0;
    int numChannels = 2;

    std::array<std::array<float, 12>, 2> detectorHistory {};
    int detectorWriteIndex = 0;

    std::vector<float> wetDelayL;
    std::vector<float> wetDelayR;
    std::vector<float> dryDelayL;
    std::vector<float> dryDelayR;
    int delayBufferSize = 2;
    int delayWriteIndex = 0;
    int lookaheadSamples = 1;

    float currentGain = 1.0f;
    float releaseAlpha = 1.0f;
    int holdSamplesRemaining = 0;
    float latestTruePeakLinear = 0.0f;
    std::atomic<float> ceilingDbTP { -1.0f };
    std::atomic<float> ceilingLinear { 0.891250938f };
};
