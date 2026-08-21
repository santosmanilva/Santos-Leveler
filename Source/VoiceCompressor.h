#pragma once

#include <algorithm>
#include <cmath>

#include "DenormalProtection.h"
#include "Constants.h"

class SantosVoiceCompressor
{
public:
    struct Parameters
    {
        bool enabled = false;
        float thresholdDb = -18.0f;
        float ratio = 3.0f;
        float attackMs = 10.0f;
        float releaseMs = 120.0f;
        float makeupDb = 0.0f;
    };

    void prepare (double newSampleRate) noexcept
    {
        sampleRate = std::max (1.0, newSampleRate);
        reset();
    }

    void reset() noexcept
    {
        envelopeDb = -100.0f;
        currentGainDb = 0.0f;
        currentMakeupDb = 0.0f;
    }

    void process (float& left, float& right, const Parameters& p) noexcept
    {
        // Stereo-linked detection: use the sum (mid) channel for envelope,
        // so both channels get the same gain reduction.
        const auto detectorLinear = (std::abs(left) + std::abs(right)) * 0.5f;
        const auto detectorDb = gainToDb (detectorLinear);

        using namespace SantosConstants;
        const auto safeAttackMs = std::clamp (p.attackMs, minCompAttackMs, maxCompAttackMs);
        const auto safeReleaseMs = std::clamp (p.releaseMs, minCompReleaseMs, maxCompReleaseMs);
        const auto envelopeAlpha = detectorDb > envelopeDb
            ? timeConstantAlpha (safeAttackMs)
            : timeConstantAlpha (safeReleaseMs);
        envelopeDb += envelopeAlpha * (detectorDb - envelopeDb);
        envelopeDb = denormalize(envelopeDb);

        float targetReductionDb = 0.0f;

        if (p.enabled)
        {
            const auto thresholdDb = std::clamp (p.thresholdDb, minCompThresholdDb, maxCompThresholdDb);
            const auto ratio = std::clamp (p.ratio, minCompRatio, maxCompRatio);
            const auto kneeDb = compressorKneeDb;
            const auto x = envelopeDb - thresholdDb;

            if (x <= -0.5f * kneeDb)
            {
                targetReductionDb = 0.0f;
            }
            else if (x >= 0.5f * kneeDb)
            {
                targetReductionDb = (thresholdDb + x / ratio) - envelopeDb;
            }
            else
            {
                const auto y = x + 0.5f * kneeDb;
                targetReductionDb = (1.0f / ratio - 1.0f) * y * y / (2.0f * kneeDb);
            }
        }

        const auto gainAlpha = targetReductionDb < currentGainDb
            ? timeConstantAlpha (safeAttackMs)
            : timeConstantAlpha (p.enabled ? safeReleaseMs : 20.0f);
        currentGainDb += gainAlpha * (targetReductionDb - currentGainDb);
        currentGainDb = denormalize(currentGainDb);

        const auto targetMakeupDb = p.enabled ? std::clamp (p.makeupDb, minCompMakeupDb, maxCompMakeupDb) : 0.0f;
        currentMakeupDb += timeConstantAlpha (20.0f) * (targetMakeupDb - currentMakeupDb);
        currentMakeupDb = denormalize(currentMakeupDb);

        const auto totalGain = dbToGain (currentGainDb + currentMakeupDb);
        left *= totalGain;
        right *= totalGain;
    }

    float getGainReductionDb() const noexcept { return std::min (0.0f, currentGainDb); }

public:
    float timeConstantAlpha (float ms) const noexcept
    {
        const auto seconds = std::max (0.000001, static_cast<double> (ms) * 0.001);
        return static_cast<float> (1.0 - std::exp (-1.0 / (seconds * sampleRate)));
    }

    static float dbToGain (float db) noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }

    static float gainToDb (float gain) noexcept
    {
        if (gain <= 1.0e-8f)
            return -100.0f;
        return std::max (-100.0f, 20.0f * std::log10 (gain));
    }

    double sampleRate = 48000.0;
    float envelopeDb = -100.0f;
    float currentGainDb = 0.0f;
    float currentMakeupDb = 0.0f;
};
