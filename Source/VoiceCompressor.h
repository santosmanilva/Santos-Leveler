#pragma once

#include <algorithm>
#include <cmath>

#include "Constants.h"
#include "DenormalProtection.h"

// Feed-forward voice compressor, stereo linked, soft knee.
//
// Topology (log domain, "smooth decoupled" peak detector applied to the gain
// computer output):
//
//   level = max(|L|, |R|)  ->  dB  ->  static soft-knee curve  ->  wanted reduction
//   release stage:  y1 = max(wanted, releaseSmoothed(y1))
//   attack stage:   y  = attackSmoothed(y1)
//
// This gives a single attack and a single release time constant, so the
// Attack / Release controls match the measured behaviour, and the steady-state
// reduction follows the static Threshold / Ratio curve.
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
        cachedAttackMs = -1.0f;
        cachedReleaseMs = -1.0f;
        makeupAlpha = timeConstantAlpha (20.0f);
        disabledReleaseAlpha = timeConstantAlpha (20.0f);
        reset();
    }

    void reset() noexcept
    {
        releaseStageDb = 0.0f;
        currentReductionDb = 0.0f;
        currentMakeupDb = 0.0f;
    }

    void process (float& left, float& right, const Parameters& p) noexcept
    {
        using namespace SantosConstants;
        updateTimeConstants (p);

        // Stereo link on the louder channel: a voice panned to one side is
        // compressed exactly as much as the same voice in the centre.
        const auto detectorLinear = std::max (std::abs (left), std::abs (right));

        float wantedReductionDb = 0.0f; // <= 0

        if (p.enabled)
        {
            const auto levelDb = gainToDb (detectorLinear);
            const auto thresholdDb = std::clamp (p.thresholdDb, minCompThresholdDb, maxCompThresholdDb);
            const auto ratio = std::clamp (p.ratio, minCompRatio, maxCompRatio);
            wantedReductionDb = staticReductionDb (levelDb, thresholdDb, ratio, compressorKneeDb);
        }

        // Work with positive "amount of reduction" values.
        const auto wanted = -wantedReductionDb;
        const auto releaseAlpha = p.enabled ? currentReleaseAlpha : disabledReleaseAlpha;

        // Release stage: instant rise, exponential fall.
        releaseStageDb = std::max (wanted, releaseStageDb + releaseAlpha * (wanted - releaseStageDb));
        releaseStageDb = denormalize (releaseStageDb);

        // Attack stage: smooths the rise with the Attack time constant.
        auto amount = -currentReductionDb;
        const auto attackAlpha = p.enabled ? currentAttackAlpha : disabledReleaseAlpha;
        amount += attackAlpha * (releaseStageDb - amount);
        currentReductionDb = denormalize (-amount);

        const auto targetMakeupDb = p.enabled
            ? std::clamp (p.makeupDb, minCompMakeupDb, maxCompMakeupDb)
            : 0.0f;
        currentMakeupDb += makeupAlpha * (targetMakeupDb - currentMakeupDb);
        currentMakeupDb = denormalize (currentMakeupDb);

        const auto totalGain = dbToGain (currentReductionDb + currentMakeupDb);
        left *= totalGain;
        right *= totalGain;
    }

    float getGainReductionDb() const noexcept { return std::min (0.0f, currentReductionDb); }

    // Static soft-knee curve. Returns the wanted gain change in dB (<= 0).
    static float staticReductionDb (float levelDb, float thresholdDb, float ratio, float kneeDb) noexcept
    {
        const auto x = levelDb - thresholdDb;
        const auto slope = 1.0f / ratio - 1.0f;

        if (x <= -0.5f * kneeDb)
            return 0.0f;

        if (x >= 0.5f * kneeDb)
            return slope * x;

        const auto y = x + 0.5f * kneeDb;
        return slope * y * y / (2.0f * kneeDb);
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

private:
    void updateTimeConstants (const Parameters& p) noexcept
    {
        using namespace SantosConstants;
        const auto attackMs = std::clamp (p.attackMs, minCompAttackMs, maxCompAttackMs);
        const auto releaseMs = std::clamp (p.releaseMs, minCompReleaseMs, maxCompReleaseMs);

        if (attackMs != cachedAttackMs)
        {
            cachedAttackMs = attackMs;
            currentAttackAlpha = timeConstantAlpha (attackMs);
        }

        if (releaseMs != cachedReleaseMs)
        {
            cachedReleaseMs = releaseMs;
            currentReleaseAlpha = timeConstantAlpha (releaseMs);
        }
    }

    float timeConstantAlpha (float ms) const noexcept
    {
        const auto seconds = std::max (0.000001, static_cast<double> (ms) * 0.001);
        return static_cast<float> (1.0 - std::exp (-1.0 / (seconds * sampleRate)));
    }

    double sampleRate = 48000.0;
    float releaseStageDb = 0.0f;     // positive amount of reduction
    float currentReductionDb = 0.0f; // <= 0
    float currentMakeupDb = 0.0f;

    float cachedAttackMs = -1.0f;
    float cachedReleaseMs = -1.0f;
    float currentAttackAlpha = 1.0f;
    float currentReleaseAlpha = 1.0f;
    float makeupAlpha = 1.0f;
    float disabledReleaseAlpha = 1.0f;
};
