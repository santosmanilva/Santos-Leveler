#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

class SantosLevelerEngine
{
public:
    // Upper bound for the Lookahead parameter. Kept here (rather than only
    // in the processor's parameter layout) so the engine's delay line and
    // the UI range can never drift out of sync.
    static constexpr float maxLookaheadMs = 20.0f;

    struct Parameters
    {
        float targetDb      = -20.0f;
        float gateDb        = -45.0f;
        float attackMs      = 12.0f;   // time constant while pulling gain DOWN (input louder than target)
        float releaseMs     = 45.0f;   // time constant while raising gain back UP (input quieter than target)
        float detectMs      = 8.0f;
        float rangeDownDb   = -9.0f;
        float rangeUpDb     = 9.0f;
        float lookaheadMs   = 8.0f;    // 0 = disabled
        float ceilingDb     = -0.3f;   // safety limiter threshold
        float outputDb      = 0.0f;
    };

    struct Telemetry
    {
        float inputDb  = -100.0f;
        float riderDb  = 0.0f;
        float outputDb = -100.0f;
        bool riderActive = false;
    };

    void prepare (double newSampleRate, int newNumChannels)
    {
        sampleRate = std::max (1.0, newSampleRate);
        numChannels = std::clamp (newNumChannels, 1, 2);
        inputDetector.prepare (sampleRate, 100.0);
        outputDetector.prepare (sampleRate, 100.0);
        controlPeriodSamples = std::max (1, static_cast<int> (std::round (sampleRate / 240.0)));
        lookaheadLine.prepare (sampleRate, maxLookaheadMs);
        reset();
    }

    void reset()
    {
        inputDetector.reset();
        outputDetector.reset();
        lookaheadLine.reset();
        controlCountdown = 0;
        currentRiderGain = 1.0f;
        targetRiderGain = 1.0f;
        currentOutputGain = 1.0f;
        limiterGain = 1.0f;
        gateOpen = false;
        riderActive = false;
        lastTelemetry = {};
    }

    Telemetry processSample (float& left, float& right, const Parameters& p)
    {
        const auto detectMs = std::clamp (p.detectMs, 1.0f, 100.0f);
        inputDetector.setWindowMs (detectMs);
        outputDetector.setWindowMs (detectMs);

        // Detection runs on the LIVE input, before it enters the lookahead
        // delay line below. That gives the gain a head start: by the time
        // the matching audio sample comes out of the delay line, the
        // smoothed rider gain has already had `lookaheadMs` to catch up.
        const auto inputRms = inputDetector.process (left, right, numChannels);
        const auto inputDb = gainToDb (inputRms);

        if (controlCountdown <= 0)
        {
            const auto errorDb = p.targetDb - inputDb;

            // Preserve the behaviour of the working MNodes v17:
            // positive and negative correction are each limited to +/-12 dB,
            // then proportionally scaled by Range Up/Down.
            const auto positive = std::clamp (errorDb, 0.0f, 12.0f)
                                * (std::clamp (p.rangeUpDb, 0.0f, 12.0f) / 12.0f);

            const auto negative = std::clamp (errorDb, -12.0f, 0.0f)
                                * (std::clamp (-p.rangeDownDb, 0.0f, 12.0f) / 12.0f);

            // Schmitt-trigger gate: opens when the input rises above Gate,
            // but only closes once it falls `gateHysteresisDb` below Gate.
            // That dead zone stops the gate chattering open/closed when the
            // signal sits right on the threshold.
            if (gateOpen)
            {
                if (inputDb < p.gateDb - gateHysteresisDb)
                    gateOpen = false;
            }
            else if (inputDb > p.gateDb)
            {
                gateOpen = true;
            }

            const auto correctionDb = gateOpen ? (positive + negative) : 0.0f;

            targetRiderGain = dbToGain (correctionDb);
            riderActive = gateOpen && std::abs (correctionDb) > 0.01f;
            controlCountdown = controlPeriodSamples;
        }
        --controlCountdown;

        // Attack = time constant while lowering gain (input louder than
        // target, correcting a loud passage down). Release = time constant
        // while raising gain back up (input quieter than target). This
        // mirrors standard compressor terminology, transposed to a rider
        // where "gain reduction" and "gain recovery" can both be active.
        const auto loweringGain = targetRiderGain < currentRiderGain;
        const auto timeMs = loweringGain ? std::clamp (p.attackMs, 2.0f, 250.0f)
                                          : std::clamp (p.releaseMs, 2.0f, 250.0f);
        const auto riderAlpha = timeConstantAlpha (timeMs);
        currentRiderGain += riderAlpha * (targetRiderGain - currentRiderGain);

        const auto wantedOutputGain = dbToGain (std::clamp (p.outputDb, -12.0f, 12.0f));
        const auto outputAlpha = timeConstantAlpha (30.0f);
        currentOutputGain += outputAlpha * (wantedOutputGain - currentOutputGain);

        // Push the live samples into the lookahead line, then read back
        // whatever arrived `lookaheadMs` ago and apply today's gain to
        // that instead of to the sample that produced it.
        lookaheadLine.push (left, numChannels > 1 ? right : left);
        const auto lookaheadSamples = lookaheadLine.millisecondsToSamples (p.lookaheadMs);
        const auto delayed = lookaheadLine.readDelayed (lookaheadSamples);

        const auto totalGain = currentRiderGain * currentOutputGain;
        auto outLeft = delayed.first * totalGain;
        auto outRight = numChannels > 1 ? delayed.second * totalGain : outLeft;

        // Fast safety limiter on the way out: instant attack so a sudden
        // transient can never exceed Ceiling, with a smoothed release so it
        // doesn't pump audibly once the peak has passed.
        applyCeilingLimiter (outLeft, outRight, p.ceilingDb);

        left = outLeft;
        right = outRight;

        const auto outputRms = outputDetector.process (left, right, numChannels);
        const auto outputDb = gainToDb (outputRms);

        lastTelemetry.inputDb = inputDb;
        lastTelemetry.riderDb = gainToDb (currentRiderGain);
        lastTelemetry.outputDb = outputDb;
        lastTelemetry.riderActive = riderActive;
        return lastTelemetry;
    }

    Telemetry getTelemetry() const noexcept { return lastTelemetry; }

    // Exposed so the processor can report accurate PDC latency to the host.
    int lookaheadSamplesFor (float lookaheadMs) const noexcept
    {
        return lookaheadLine.millisecondsToSamples (lookaheadMs);
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
            const auto square = channels > 1
                ? 0.5f * (left * left + right * right)
                : left * left;

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

    // Small stereo circular buffer used for the lookahead delay. Detection
    // reads the signal before it goes in; the gain stage reads it back out
    // `lookaheadMs` later.
    class LookaheadLine
    {
    public:
        void prepare (double sr, float maxMs)
        {
            sampleRate = std::max (1.0, sr);
            maxSamples = std::max (1, static_cast<int> (std::ceil (sampleRate * static_cast<double> (maxMs) * 0.001)));
            bufferL.assign (static_cast<std::size_t> (maxSamples), 0.0f);
            bufferR.assign (static_cast<std::size_t> (maxSamples), 0.0f);
            reset();
        }

        void reset()
        {
            std::fill (bufferL.begin(), bufferL.end(), 0.0f);
            std::fill (bufferR.begin(), bufferR.end(), 0.0f);
            writeIndex = 0;
        }

        int millisecondsToSamples (float ms) const noexcept
        {
            const auto maxMsAllowed = static_cast<float> (maxSamples - 1) * 1000.0f / static_cast<float> (sampleRate);
            const auto clampedMs = std::clamp (ms, 0.0f, maxMsAllowed);
            return std::clamp (static_cast<int> (std::round (sampleRate * static_cast<double> (clampedMs) * 0.001)), 0, maxSamples - 1);
        }

        void push (float l, float r)
        {
            bufferL[static_cast<std::size_t> (writeIndex)] = l;
            bufferR[static_cast<std::size_t> (writeIndex)] = r;
            writeIndex = (writeIndex + 1) % maxSamples;
        }

        std::pair<float, float> readDelayed (int delaySamples) const
        {
            const auto clampedDelay = std::clamp (delaySamples, 0, maxSamples - 1);
            const auto readIndex = (writeIndex - 1 - clampedDelay + maxSamples) % maxSamples;
            return { bufferL[static_cast<std::size_t> (readIndex)], bufferR[static_cast<std::size_t> (readIndex)] };
        }

    private:
        double sampleRate = 48000.0;
        int maxSamples = 1;
        int writeIndex = 0;
        std::vector<float> bufferL, bufferR;
    };

    float timeConstantAlpha (float ms) const noexcept
    {
        const auto seconds = std::max (0.000001, static_cast<double> (ms) * 0.001);
        return static_cast<float> (1.0 - std::exp (-1.0 / (seconds * sampleRate)));
    }

    void applyCeilingLimiter (float& outLeft, float& outRight, float ceilingDb) noexcept
    {
        const auto ceilingLinear = dbToGain (ceilingDb);
        const auto peak = std::max (std::abs (outLeft), std::abs (outRight));
        const auto instantNeeded = (peak > ceilingLinear && peak > 1.0e-8f) ? (ceilingLinear / peak) : 1.0f;

        if (instantNeeded < limiterGain)
        {
            // Instant attack: clamp down immediately so this sample never
            // exceeds the ceiling, however hard it hits.
            limiterGain = instantNeeded;
        }
        else
        {
            // Smoothed release back toward whatever headroom is currently
            // needed (1.0 once nothing is being limited).
            const auto releaseAlpha = timeConstantAlpha (limiterReleaseMs);
            limiterGain += releaseAlpha * (instantNeeded - limiterGain);
        }

        limiterGain = std::clamp (limiterGain, 0.0f, 1.0f);
        outLeft *= limiterGain;
        outRight *= limiterGain;
    }

    static constexpr float gateHysteresisDb = 3.0f;
    static constexpr float limiterReleaseMs = 60.0f;

    double sampleRate = 48000.0;
    int numChannels = 2;
    int controlPeriodSamples = 200;
    int controlCountdown = 0;
    float targetRiderGain = 1.0f;
    float currentRiderGain = 1.0f;
    float currentOutputGain = 1.0f;
    float limiterGain = 1.0f;
    bool gateOpen = false;
    bool riderActive = false;
    SlidingRms inputDetector;
    SlidingRms outputDetector;
    LookaheadLine lookaheadLine;
    Telemetry lastTelemetry;
};
DSP: lookahead, limiter, attack/release, gate hysteresis
