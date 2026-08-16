#pragma once

#include <algorithm>
#include <cmath>

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
    }

    void process (float& left, float& right, const Parameters& p) noexcept
    {
        if (! p.enabled)
        {
            currentGainDb += timeConstantAlpha (20.0f) * (0.0f - currentGainDb);
            return;
        }

        const auto detectorLinear = std::max (std::abs (left), std::abs (right));
        const auto detectorDb = gainToDb (detectorLinear);

        const auto safeAttackMs = std::clamp (p.attackMs, 0.5f, 100.0f);
        const auto safeReleaseMs = std::clamp (p.releaseMs, 20.0f, 1000.0f);
        const auto envelopeAlpha = detectorDb > envelopeDb
            ? timeConstantAlpha (safeAttackMs)
            : timeConstantAlpha (safeReleaseMs);
        envelopeDb += envelopeAlpha * (detectorDb - envelopeDb);

        const auto thresholdDb = std::clamp (p.thresholdDb, -36.0f, 0.0f);
        const auto ratio = std::clamp (p.ratio, 1.0f, 10.0f);
        constexpr float kneeDb = 6.0f;
        const auto x = envelopeDb - thresholdDb;

        float targetReductionDb = 0.0f;
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

        const auto gainAlpha = targetReductionDb < currentGainDb
            ? timeConstantAlpha (safeAttackMs)
            : timeConstantAlpha (safeReleaseMs);
        currentGainDb += gainAlpha * (targetReductionDb - currentGainDb);

        const auto makeupDb = std::clamp (p.makeupDb, 0.0f, 12.0f);
        const auto totalGain = dbToGain (currentGainDb + makeupDb);
        left *= totalGain;
        right *= totalGain;
    }

    float getGainReductionDb() const noexcept { return std::min (0.0f, currentGainDb); }

private:
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
};
