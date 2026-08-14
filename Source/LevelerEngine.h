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
        float targetDb      = -20.0f;
        float gateDb        = -45.0f;
        float speedMs       = 15.0f;
        float detectMs      = 8.0f;
        float rangeDownDb   = -9.0f;
        float rangeUpDb     = 9.0f;
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
        reset();
    }

    void reset()
    {
        inputDetector.reset();
        outputDetector.reset();
        controlCountdown = 0;
        currentRiderGain = 1.0f;
        targetRiderGain = 1.0f;
        currentOutputGain = 1.0f;
        lastTelemetry = {};
    }

    Telemetry processSample(float& left, float& right, const Parameters& p)
    {
        const auto detectorLeft = left;
        const auto detectorRight = right;

        return processSampleLookahead(detectorLeft, detectorRight, left, right, p);
    }

    Telemetry processSampleLookahead(float detectorLeft,
        float detectorRight,
        float& left,
        float& right,
        const Parameters& p)
    {
        const auto detectMs = std::clamp(p.detectMs, 1.0f, 100.0f);
        inputDetector.setWindowMs(detectMs);
        outputDetector.setWindowMs(detectMs);

        // El detector analiza la señal ACTUAL, no la retrasada.
        const auto inputRms = inputDetector.process(detectorLeft,
            detectorRight,
            numChannels);
        const auto inputDb = gainToDb(inputRms);

        if (controlCountdown <= 0)
        {
            const auto errorDb = p.targetDb - inputDb;

            const auto positive = std::clamp(errorDb, 0.0f, 12.0f)
                * (std::clamp(p.rangeUpDb, 0.0f, 12.0f) / 12.0f);

            const auto negative = std::clamp(errorDb, -12.0f, 0.0f)
                * (std::clamp(-p.rangeDownDb, 0.0f, 12.0f) / 12.0f);

            const auto active = inputDb > p.gateDb;
            const auto correctionDb = active ? (positive + negative) : 0.0f;

            targetRiderGain = dbToGain(correctionDb);
            riderActive = active && std::abs(correctionDb) > 0.01f;
            controlCountdown = controlPeriodSamples;
        }

        --controlCountdown;

        const auto safeSpeedMs = std::clamp(p.speedMs, 2.0f, 250.0f);
        const auto riderAlpha = timeConstantAlpha(safeSpeedMs);

        currentRiderGain += riderAlpha * (targetRiderGain - currentRiderGain);

        const auto wantedOutputGain =
            dbToGain(std::clamp(p.outputDb, -12.0f, 12.0f));

        const auto outputAlpha = timeConstantAlpha(30.0f);

        currentOutputGain += outputAlpha
            * (wantedOutputGain - currentOutputGain);

        const auto totalGain = currentRiderGain * currentOutputGain;

        // La ganancia se aplica a la señal RETRASADA.
        left *= totalGain;

        if (numChannels > 1)
            right *= totalGain;
        else
            right = left;

        const auto outputRms =
            outputDetector.process(left, right, numChannels);

        const auto outputDb = gainToDb(outputRms);

        lastTelemetry.inputDb = inputDb;
        lastTelemetry.riderDb = gainToDb(currentRiderGain);
        lastTelemetry.outputDb = outputDb;
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
    SlidingRms inputDetector;
    SlidingRms outputDetector;
    Telemetry lastTelemetry;
};
