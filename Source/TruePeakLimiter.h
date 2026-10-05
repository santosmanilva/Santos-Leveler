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
        // Never shorter than the detector delay plus a 16 sample ramp, even at very low sample rates.
        lookaheadSamples = std::max (detectorDelaySamples + 16,
                                     static_cast<int> (std::round (sampleRate * (truePeakLookaheadMs * 0.001))));
        delayBufferSize = lookaheadSamples + 1;

        wetDelayL.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        wetDelayR.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        dryDelayL.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);
        dryDelayR.assign (static_cast<std::size_t> (delayBufferSize), 0.0f);

        releaseAlpha = static_cast<float> (
            1.0 - std::exp (-1.0 / (truePeakReleaseMs * 0.001 * sampleRate)));

        // Gain path: sliding minimum over the lookahead window, release, then a
        // moving average that turns the gain drop into a short ramp. The ramp is
        // shorter than the lookahead by the detector delay, so the full
        // reduction is reached before the detected peak leaves the delay line.
        minWindowSize = lookaheadSamples + 1;
        rampLength = std::max (1, lookaheadSamples - detectorDelaySamples);
        buildDetectorCoefficients();
        requiredGainHistory.assign (static_cast<std::size_t> (minWindowSize), 1.0f);
        rampHistory.assign (static_cast<std::size_t> (rampLength), 1.0f);

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
        detectorWriteIndex = 0;

        std::fill (wetDelayL.begin(), wetDelayL.end(), 0.0f);
        std::fill (wetDelayR.begin(), wetDelayR.end(), 0.0f);
        std::fill (dryDelayL.begin(), dryDelayL.end(), 0.0f);
        std::fill (dryDelayR.begin(), dryDelayR.end(), 0.0f);

        std::fill (requiredGainHistory.begin(), requiredGainHistory.end(), 1.0f);
        std::fill (rampHistory.begin(), rampHistory.end(), 1.0f);
        requiredGainWriteIndex = 0;
        rampWriteIndex = 0;
        rampSum = static_cast<double> (rampLength);
        releasedGain = 1.0f;

        detectorWriteIndex = 0;
        delayWriteIndex = 0;
        currentGain = 1.0f;
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
        // A 0.1 dB safety margin covers the inter-sample change caused by the gain
        // ramp itself and the residual error of the interpolating detector.
        const auto currentCeilingLinear = ceilingLinear.load (std::memory_order_acquire) * ceilingSafetyFactor;
        const auto truePeak = detectTruePeak (wetL, wetR, currentCeilingLinear);
        latestTruePeakLinear = denormalize(truePeak);
        const auto requiredGain = truePeak > currentCeilingLinear
            ? std::clamp (currentCeilingLinear / std::max (truePeak, 1.0e-9f), 0.0f, 1.0f)
            : 1.0f;

        currentGain = computeGain (requiredGain);

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
    float computeGain (float requiredGain) noexcept
    {
        // 1) Sliding minimum of the required gain over the lookahead window.
        requiredGainHistory[static_cast<std::size_t> (requiredGainWriteIndex)] = requiredGain;
        if (++requiredGainWriteIndex >= minWindowSize)
            requiredGainWriteIndex = 0;

        float windowMinimum = 1.0f;
        for (const auto g : requiredGainHistory)
            windowMinimum = std::min (windowMinimum, g);

        // 2) Instant attack / exponential release.
        if (windowMinimum < releasedGain)
            releasedGain = windowMinimum;
        else
            releasedGain += releaseAlpha * (windowMinimum - releasedGain);

        if (releasedGain > 0.999999f)
            releasedGain = 1.0f;

        // 3) Moving average: turns the instant attack into a smooth ramp.
        auto& oldest = rampHistory[static_cast<std::size_t> (rampWriteIndex)];
        rampSum += static_cast<double> (releasedGain) - static_cast<double> (oldest);
        oldest = releasedGain;
        if (++rampWriteIndex >= rampLength)
        {
            rampWriteIndex = 0;
            // Refresh the running sum once per cycle so rounding never accumulates.
            rampSum = 0.0;
            for (const auto g : rampHistory)
                rampSum += static_cast<double> (g);
        }

        return std::clamp (static_cast<float> (rampSum / static_cast<double> (rampLength)), 0.0f, 1.0f);
    }

    static double besselI0 (double x) noexcept
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 50; ++k)
        {
            const auto half = x / (2.0 * k);
            term *= half * half;
            sum += term;
        }
        return sum;
    }

    // 8x polyphase windowed-sinc (Kaiser) interpolator used only for peak detection.
    // Phase p evaluates the signal p/8 of a sample after the sample that is
    // detectorDelaySamples behind the newest one, so the detector sees the whole
    // interval between two samples. The 4x / 12-tap filter of BS.1770 can miss
    // 0.5 dB or more on noisy, high-frequency content such as sibilants.
    void buildDetectorCoefficients() noexcept
    {
        constexpr double pi = 3.14159265358979323846;
        constexpr double kaiserBeta = 8.0;
        const auto beta0 = besselI0 (kaiserBeta);
        lebesgueConstant = 0.0f;

        for (int phase = 0; phase < detectorPhases; ++phase)
        {
            double absoluteSum = 0.0;
            for (int tap = 0; tap < detectorTaps; ++tap)
            {
                const auto x = static_cast<double> (tap - detectorDelaySamples)
                             + static_cast<double> (phase) / detectorPhases;
                const auto sinc = std::abs (x) < 1.0e-12 ? 1.0 : std::sin (pi * x) / (pi * x);
                const auto r = x / static_cast<double> (detectorDelaySamples);
                const auto window = std::abs (r) >= 1.0 ? 0.0 : besselI0 (kaiserBeta * std::sqrt (1.0 - r * r)) / beta0;
                const auto coefficient = sinc * window;
                // Stored oldest-first, matching the contiguous layout of detectorHistory.
                detectorCoefficients[static_cast<std::size_t> (phase)][static_cast<std::size_t> (detectorTaps - 1 - tap)] = static_cast<float> (coefficient);
                absoluteSum += std::abs (coefficient);
            }
            lebesgueConstant = std::max (lebesgueConstant, static_cast<float> (absoluteSum));
        }
    }

    float detectTruePeak (float left, float right, float ceilingToCheck) noexcept
    {
        // Each sample is written twice so the last detectorTaps samples are always one
        // contiguous run (oldest first) starting at detectorWriteIndex + 1.
        const auto write = static_cast<std::size_t> (detectorWriteIndex);
        detectorHistory[0][write] = left;
        detectorHistory[0][write + detectorTaps] = left;
        detectorHistory[1][write] = right;
        detectorHistory[1][write + detectorTaps] = right;

        float maximum = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const float* window = detectorHistory[static_cast<std::size_t> (channel)].data() + write + 1;

            // |interpolated value| <= lebesgueConstant * (largest sample in the window),
            // so a quiet window cannot reach the ceiling and the filter can be skipped.
            float windowPeak = 0.0f;
            for (int i = 0; i < detectorTaps; ++i)
                windowPeak = std::max (windowPeak, std::abs (window[i]));

            if (windowPeak * lebesgueConstant <= ceilingToCheck)
            {
                maximum = std::max (maximum, windowPeak);
                continue;
            }

            // Phase 0 is the sample detectorDelaySamples behind the newest one.
            maximum = std::max (maximum, std::abs (window[detectorTaps - 1 - detectorDelaySamples]));

            for (int phase = 1; phase < detectorPhases; ++phase)
            {
                const float* coefficients = detectorCoefficients[static_cast<std::size_t> (phase)].data();
                float accumulator[8] {};
                for (int tap = 0; tap < detectorTaps; tap += 8)
                    for (int lane = 0; lane < 8; ++lane)
                        accumulator[lane] += window[tap + lane] * coefficients[tap + lane];

                const auto value = ((accumulator[0] + accumulator[4]) + (accumulator[1] + accumulator[5]))
                                 + ((accumulator[2] + accumulator[6]) + (accumulator[3] + accumulator[7]));
                maximum = std::max (maximum, std::abs (value));
            }
        }

        detectorWriteIndex = (detectorWriteIndex + 1) & detectorMask;
        return maximum;
    }

    double sampleRate = 48000.0;
    int numChannels = 2;

    static constexpr int detectorTaps = 64;
    static constexpr int detectorMask = detectorTaps - 1;
    static constexpr int detectorPhases = 8;
    static constexpr int detectorDelaySamples = detectorTaps / 2;
    std::array<std::array<float, 2 * detectorTaps>, 2> detectorHistory {};
    std::array<std::array<float, detectorTaps>, detectorPhases> detectorCoefficients {};
    float lebesgueConstant = 1.0f;
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

    std::vector<float> requiredGainHistory;
    std::vector<float> rampHistory;
    int minWindowSize = 2;
    int rampLength = 1;
    int requiredGainWriteIndex = 0;
    int rampWriteIndex = 0;
    double rampSum = 1.0;
    float releasedGain = 1.0f;
    static constexpr float ceilingSafetyFactor = 0.988553f; // -0.1 dB
    float latestTruePeakLinear = 0.0f;
    std::atomic<float> ceilingDbTP { -1.0f };
    std::atomic<float> ceilingLinear { 0.891250938f };
};