#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

class SantosLevelerEngine
{
public:
    struct Parameters
    {
        float targetDb = -20.0f;
        float gateDb = -45.0f;
        float speedMs = 15.0f;
        float detectMs = 8.0f;
        float lookaheadMs = 30.0f;
        float holdMs = 100.0f;
        float releaseMs = 500.0f;
        float peakThresholdDb = -9.0f;
        float rangeDownDb = -9.0f;
        float rangeUpDb = 9.0f;
        float outputDb = 0.0f;
    };

    struct Telemetry
    {
        float inputDb = -100.0f;
        float fastDb = -100.0f;
        float slowDb = -100.0f;
        float controlDb = -100.0f;
        float requestedRiderDb = 0.0f;
        float effectiveRiderDb = 0.0f;
        float riderDb = 0.0f;
        float peakEnvelopeDb = -100.0f;
        float peakReductionDb = 0.0f;
        float peakDb = 0.0f;
        float outputDb = -100.0f;
        bool gateActive = false;
        bool riderActive = false;
    };

    void prepare (double newSampleRate, int newNumChannels)
    {
        sampleRate = std::max (1.0, newSampleRate);
        numChannels = std::clamp (newNumChannels, 1, 2);

        inputFastDetector.prepare (sampleRate, 100.0);
        inputSlowDetector.prepare (sampleRate, 300.0);
        outputDetector.prepare (sampleRate, 100.0);

        controlPeriodSamples = std::max (1, static_cast<int> (std::round (sampleRate / 240.0)));
        reset();
    }

    void reset()
    {
        inputFastDetector.reset();
        inputSlowDetector.reset();
        outputDetector.reset();
        controlCountdown = 0;
        currentRiderGain = 1.0f;
        targetRiderGain = 1.0f;
        currentOutputGain = 1.0f;
        latestRequestedCorrectionDb = 0.0f;
        heldCorrectionDb = 0.0f;
        holdSamplesRemaining = 0;
        holdActive = false;
        detectorActive = false;
        gateCloseSamplesRemaining = 0;

        peakEnvelopeDb = -100.0f;
        peakReductionDb = 0.0f;
        currentPeakGain = 1.0f;
        riderActive = false;
        lastTelemetry = {};
    }

    Telemetry processSample (float& left, float& right, const Parameters& p)
    {
        const auto detectorLeft = left;
        const auto detectorRight = right;
        return processSampleLookahead (detectorLeft, detectorRight, left, right, p);
    }

    Telemetry processSampleLookahead (float detectorLeft,
                                      float detectorRight,
                                      float& left,
                                      float& right,
                                      const Parameters& p)
    {
        const auto fastDetectMs = std::clamp (p.detectMs, 1.0f, 100.0f);
        const auto slowDetectMs = std::clamp (fastDetectMs * 12.0f, 70.0f, 250.0f);

        inputFastDetector.setWindowMs (fastDetectMs);
        inputSlowDetector.setWindowMs (slowDetectMs);
        outputDetector.setWindowMs (fastDetectMs);

        const auto fastRms = inputFastDetector.process (detectorLeft, detectorRight, numChannels);
        const auto slowRms = inputSlowDetector.process (detectorLeft, detectorRight, numChannels);

        const auto fastDb = gainToDb (fastRms);
        const auto slowDb = gainToDb (slowRms);

        const auto detectorDeltaDb = fastDb - slowDb;
        const auto fastWeight = detectorDeltaDb >= 0.0f ? 0.78f : 0.22f;
        const auto controlInputDb = slowDb + detectorDeltaDb * fastWeight;
        const auto inputDb = controlInputDb;

        const auto instantaneousPeak =
            numChannels > 1
                ? std::max (std::abs (detectorLeft), std::abs (detectorRight))
                : std::abs (detectorLeft);

        const auto instantaneousPeakDb = gainToDb (instantaneousPeak);

        if (instantaneousPeakDb > peakEnvelopeDb)
            peakEnvelopeDb = instantaneousPeakDb;
        else
            peakEnvelopeDb += timeConstantAlpha (80.0f) * (instantaneousPeakDb - peakEnvelopeDb);

        if (controlCountdown <= 0)
        {
            const auto errorDb = p.targetDb - controlInputDb;
            const auto minCorrectionDb = std::clamp (p.rangeDownDb, -12.0f, 0.0f);
            const auto maxCorrectionDb = std::clamp (p.rangeUpDb, 0.0f, 12.0f);

            // Smart Gate: the knob remains the OPEN threshold. Once active, the
            // detector must fall 3 dB below it before a close is even considered.
            // A short grace period then ignores tiny gaps between syllables/words.
            const auto gateOpenDb = std::clamp (p.gateDb, -70.0f, -25.0f);
            const auto gateCloseDb = std::max (-100.0f, gateOpenDb - 3.0f);
            constexpr float gateCloseGraceMs = 80.0f;
            const auto gateCloseGraceSamples = std::max (1, static_cast<int> (
                std::round (sampleRate * static_cast<double> (gateCloseGraceMs) * 0.001)));

            if (! detectorActive)
            {
                if (controlInputDb >= gateOpenDb)
                {
                    detectorActive = true;
                    gateCloseSamplesRemaining = gateCloseGraceSamples;
                }
            }
            else
            {
                if (controlInputDb > gateCloseDb)
                {
                    gateCloseSamplesRemaining = gateCloseGraceSamples;
                }
                else
                {
                    gateCloseSamplesRemaining = std::max (
                        0,
                        gateCloseSamplesRemaining - controlPeriodSamples);

                    if (gateCloseSamplesRemaining == 0)
                        detectorActive = false;
                }
            }

            const auto rawCorrectionDb =
                std::clamp (errorDb, minCorrectionDb, maxCorrectionDb);

            auto preservedCorrectionDb = rawCorrectionDb;

            // Preserve Dynamics V2 prototype:
            // A fast rise above the slow phrase envelope is treated as a likely
            // emphasis/transient. Only downward Rider correction is softened,
            // leaving sustained loudness to be levelled normally once SLOW catches up.
            if (detectorActive && rawCorrectionDb < 0.0f)
            {
                constexpr float preserveStartDeltaDb = 3.0f;
                constexpr float preserveFullDeltaDb = 12.0f;
                constexpr float maxPreserveAmount = 0.35f;

                const auto transientDeltaDb = std::max (0.0f, detectorDeltaDb);
                const auto preserveStrength = std::clamp (
                    (transientDeltaDb - preserveStartDeltaDb)
                        / (preserveFullDeltaDb - preserveStartDeltaDb),
                    0.0f,
                    1.0f);

                preservedCorrectionDb *= (1.0f - maxPreserveAmount * preserveStrength);
            }

            const auto newCorrectionDb =
                detectorActive ? preservedCorrectionDb : 0.0f;

            latestRequestedCorrectionDb = newCorrectionDb;

            constexpr float epsilonDb = 0.05f;
            const auto heldNearZero = std::abs (heldCorrectionDb) < epsilonDb;
            const auto newNearZero = std::abs (newCorrectionDb) < epsilonDb;

            const bool sameDirection =
                heldNearZero || newNearZero
                || ((heldCorrectionDb > 0.0f) == (newCorrectionDb > 0.0f));

            const bool relaxingTowardUnity =
                sameDirection
                && std::abs (newCorrectionDb) < std::abs (heldCorrectionDb) - epsilonDb;

            const bool directionChanged =
                ! heldNearZero && ! newNearZero && ! sameDirection;

            if (directionChanged || ! relaxingTowardUnity)
            {
                heldCorrectionDb = newCorrectionDb;
                holdActive = false;
                holdSamplesRemaining = 0;
            }
            else if (! holdActive)
            {
                const auto safeHoldMs = std::clamp (p.holdMs, 0.0f, 1000.0f);
                holdSamplesRemaining = static_cast<int> (
                    std::round (sampleRate * static_cast<double> (safeHoldMs) * 0.001));
                holdActive = holdSamplesRemaining > 0;
            }

            controlCountdown = controlPeriodSamples;
        }

        --controlCountdown;

        if (holdActive)
        {
            if (holdSamplesRemaining > 0)
                --holdSamplesRemaining;

            if (holdSamplesRemaining <= 0)
            {
                holdActive = false;
                heldCorrectionDb = latestRequestedCorrectionDb;
            }
        }

        const auto effectiveCorrectionDb =
            holdActive ? heldCorrectionDb : latestRequestedCorrectionDb;

        if (! holdActive)
            heldCorrectionDb = latestRequestedCorrectionDb;

        targetRiderGain = dbToGain (effectiveCorrectionDb);

        const auto currentRiderDb = gainToDb (currentRiderGain);
        constexpr float smoothingEpsilonDb = 0.05f;

        const auto currentNearZero = std::abs (currentRiderDb) < smoothingEpsilonDb;
        const auto targetNearZero = std::abs (effectiveCorrectionDb) < smoothingEpsilonDb;

        const bool smoothingSameDirection =
            currentNearZero || targetNearZero
            || ((currentRiderDb > 0.0f) == (effectiveCorrectionDb > 0.0f));

        const bool movingTowardUnity =
            smoothingSameDirection
            && std::abs (effectiveCorrectionDb) < std::abs (currentRiderDb) - smoothingEpsilonDb;

        const auto safeSpeedMs = std::clamp (p.speedMs, 2.0f, 250.0f);
        const auto safeReleaseMs = std::clamp (p.releaseMs, 50.0f, 3000.0f);

        const auto riderAlpha =
            movingTowardUnity ? timeConstantAlpha (safeReleaseMs)
                              : timeConstantAlpha (safeSpeedMs);

        currentRiderGain += riderAlpha * (targetRiderGain - currentRiderGain);

        const auto peakThresholdDb = std::clamp (p.peakThresholdDb, -18.0f, -1.0f);
        const auto predictedPeakDb = peakEnvelopeDb + gainToDb (currentRiderGain);

        if (predictedPeakDb > peakThresholdDb)
            peakReductionDb = std::clamp (peakThresholdDb - predictedPeakDb, -9.0f, 0.0f);
        else
            peakReductionDb = 0.0f;

        const auto targetPeakGain = dbToGain (peakReductionDb);
        const bool needsPeakReduction = targetPeakGain < currentPeakGain;

        const auto peakAlpha =
            needsPeakReduction ? timeConstantAlpha (1.0f)
                               : timeConstantAlpha (100.0f);

        currentPeakGain += peakAlpha * (targetPeakGain - currentPeakGain);

        const auto wantedOutputGain = dbToGain (std::clamp (p.outputDb, -12.0f, 12.0f));
        currentOutputGain += timeConstantAlpha (30.0f) * (wantedOutputGain - currentOutputGain);

        const auto totalGain = currentRiderGain * currentPeakGain * currentOutputGain;

        left *= totalGain;

        if (numChannels > 1)
            right *= totalGain;
        else
            right = left;

        const auto outputRms = outputDetector.process (left, right, numChannels);
        const auto outputDb = gainToDb (outputRms);

        lastTelemetry.inputDb = inputDb;
        lastTelemetry.fastDb = fastDb;
        lastTelemetry.slowDb = slowDb;
        lastTelemetry.controlDb = controlInputDb;
        lastTelemetry.requestedRiderDb = latestRequestedCorrectionDb;
        lastTelemetry.effectiveRiderDb = effectiveCorrectionDb;
        lastTelemetry.riderDb = gainToDb (currentRiderGain);
        lastTelemetry.peakEnvelopeDb = peakEnvelopeDb;
        lastTelemetry.peakReductionDb = peakReductionDb;
        lastTelemetry.peakDb = gainToDb (currentPeakGain);
        lastTelemetry.outputDb = outputDb;
        lastTelemetry.gateActive = detectorActive;

        riderActive = detectorActive || std::abs (lastTelemetry.riderDb) > 0.05f;
        lastTelemetry.riderActive = riderActive;

        return lastTelemetry;
    }

    Telemetry getTelemetry() const noexcept { return lastTelemetry; }

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
    class SlidingRms
    {
    public:
        void prepare (double sr, double maxWindowMs)
        {
            sampleRate = std::max (1.0, sr);
            maxSamples = std::max (1, static_cast<int> (std::ceil (sampleRate * maxWindowMs * 0.001)));
            history.assign (static_cast<std::size_t> (maxSamples), 0.0f);
            reset();
        }

        void reset()
        {
            std::fill (history.begin(), history.end(), 0.0f);
            writeIndex = 0;
            filled = 0;
            windowSamples = 1;
            sumSquares = 0.0;
        }

        void setWindowMs (float ms)
        {
            const auto newWindow = std::clamp (
                static_cast<int> (std::round (sampleRate * static_cast<double> (ms) * 0.001)),
                1, maxSamples);

            if (newWindow == windowSamples)
                return;

            windowSamples = newWindow;
            recomputeSum();
        }

        float process (float left, float right, int channels)
        {
            const auto square = channels > 1 ? 0.5f * (left * left + right * right) : left * left;

            if (filled >= windowSamples)
            {
                const auto oldIndex = (writeIndex - windowSamples + maxSamples) % maxSamples;
                sumSquares -= static_cast<double> (history[static_cast<std::size_t> (oldIndex)]);
            }
            else
            {
                ++filled;
            }

            history[static_cast<std::size_t> (writeIndex)] = square;
            sumSquares += static_cast<double> (square);
            writeIndex = (writeIndex + 1) % maxSamples;

            const auto denominator = std::max (1, std::min (filled, windowSamples));
            return std::sqrt (static_cast<float> (std::max (0.0, sumSquares) / denominator));
        }

    private:
        void recomputeSum()
        {
            sumSquares = 0.0;
            const auto count = std::min (filled, windowSamples);
            for (int i = 1; i <= count; ++i)
            {
                const auto idx = (writeIndex - i + maxSamples) % maxSamples;
                sumSquares += history[static_cast<std::size_t> (idx)];
            }
        }

        double sampleRate = 48000.0;
        int maxSamples = 1;
        int windowSamples = 1;
        int writeIndex = 0;
        int filled = 0;
        double sumSquares = 0.0;
        std::vector<float> history;
    };

    float timeConstantAlpha (float ms) const noexcept
    {
        const auto seconds = std::max (0.000001, static_cast<double> (ms) * 0.001);
        return static_cast<float> (1.0 - std::exp (-1.0 / (seconds * sampleRate)));
    }

    double sampleRate = 48000.0;
    int numChannels = 2;
    int controlPeriodSamples = 200;
    int controlCountdown = 0;
    float targetRiderGain = 1.0f;
    float currentRiderGain = 1.0f;
    float currentOutputGain = 1.0f;
    bool riderActive = false;
    float latestRequestedCorrectionDb = 0.0f;
    float heldCorrectionDb = 0.0f;
    int holdSamplesRemaining = 0;
    bool holdActive = false;
    bool detectorActive = false;
    int gateCloseSamplesRemaining = 0;
    float peakEnvelopeDb = -100.0f;
    float peakReductionDb = 0.0f;
    float currentPeakGain = 1.0f;
    SlidingRms inputFastDetector;
    SlidingRms inputSlowDetector;
    SlidingRms outputDetector;
    Telemetry lastTelemetry;
};
