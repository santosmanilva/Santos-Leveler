#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

class SantosTruePeakLimiter
{
public:
    void prepare (double newSampleRate, int newNumChannels)
    {
        sampleRate = std::max (1.0, newSampleRate);
        numChannels = std::clamp (newNumChannels, 1, 2);

        lookaheadSamples = std::max (1, static_cast<int> (std::round (sampleRate * 0.001)));
        delayBufferSize = lookaheadSamples + 1;

        wetDelayL.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        wetDelayR.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        dryDelayL.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        dryDelayR.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);

        constexpr double releaseMs = 120.0;
        releaseAlpha = static_cast<float> (
            1.0 - std::exp (-1.0 / (releaseMs * 0.001 * sampleRate)));

        reset();
    }

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
        latestTruePeakLinear = truePeak;

        constexpr float ceilingDbTP = -1.0f;
        constexpr float ceilingLinear = 0.891250938f;

        const auto requiredGain = truePeak > ceilingLinear
            ? std::clamp (ceilingLinear / std::max (truePeak, 1.0e-9f), 0.0f, 1.0f)
            : 1.0f;

        // Instant attack is safe because the audio path is delayed by 1 ms.
        // Hold covers the lookahead plus the ~6-sample group delay of the
        // 48-tap / 4-phase ITU interpolation filter before release begins.
        if (requiredGain < currentGain)
            currentGain = requiredGain;

        if (requiredGain < 1.0f)
            holdSamplesRemaining = std::max (holdSamplesRemaining, lookaheadSamples + 6);

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
                             * coefficients[static_cast<std::size_t> (tap)]
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

    // Recommendation ITU-R BS.1770-5, Annex 2:
    // order-48, four-phase FIR interpolator for 4x true-peak estimation.
    static constexpr std::array<std::array<float, 4>, 12> coefficients {{
        {{  0.0017089843750f, -0.0291748046875f, -0.0189208984375f, -0.0083007812500f }},
        {{  0.0109863281250f,  0.0292968750000f,  0.0330810546875f,  0.0148925781250f }},
        {{ -0.0196533203125f, -0.0517578125000f, -0.0582275390625f, -0.0266113281250f }},
        {{  0.0332031250000f,  0.0891113281250f,  0.1015625000000f,  0.0476074218750f }},
        {{ -0.0594482421875f, -0.1665039062500f, -0.2003173828125f, -0.1022949218750f }},
        {{  0.1373291015625f,  0.4650878906250f,  0.7797851562500f,  0.9721679687500f }},
        {{  0.9721679687500f,  0.7797851562500f,  0.4650878906250f,  0.1373291015625f }},
        {{ -0.1022949218750f, -0.2003173828125f, -0.1665039062500f, -0.0594482421875f }},
        {{  0.0476074218750f,  0.1015625000000f,  0.0891113281250f,  0.0332031250000f }},
        {{ -0.0266113281250f, -0.0582275390625f, -0.0517578125000f, -0.0196533203125f }},
        {{  0.0148925781250f,  0.0330810546875f,  0.0292968750000f,  0.0109863281250f }},
        {{ -0.0083007812500f, -0.0189208984375f, -0.0291748046875f,  0.0017089843750f }}
    }};

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
};
