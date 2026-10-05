#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "DenormalProtection.h"
#include "TruePeakCoefficients.h"

class SantosLoudnessMeter
{
public:
    void prepare (double newSampleRate, int newNumChannels)
    {
        sampleRate = std::max (1.0, newSampleRate);
        numChannels = std::clamp (newNumChannels, 1, 2);
        samplesPer100ms = std::max (1, static_cast<int> (std::round (sampleRate * 0.1)));

        for (auto& channel : channels)
        {
            channel.stage1.setFromAnalog (sampleRate,
                                          1.5848647011308556,
                                          18886.91437802888,
                                          112594507.26979107,
                                          1.0,
                                          15004.846526655716,
                                          112594507.26978934);
            channel.stage2.setFromAnalog (sampleRate,
                                          1.0049948987146884,
                                          0.0,
                                          0.0,
                                          1.0,
                                          478.91221140844294,
                                          57414.25935878171);
        }

        // Integrated loudness uses a fixed-size histogram of the gated 400 ms
        // blocks, allocated here and never resized on the audio thread.
        integratedEnergyHistogram.assign (integratedHistogramBins, 0.0);
        integratedCountHistogram.assign (integratedHistogramBins, 0);

        resetAll();
    }

    void resetAll() noexcept
    {
        for (auto& channel : channels)
        {
            channel.stage1.reset();
            channel.stage2.reset();
        }

        current100msSquares.fill (0.0);
        block100msHistory.fill ({});
        blockHistoryWrite = 0;
        blockHistoryCount = 0;
        samplesInCurrent100ms = 0;
        clearIntegratedHistogram();
        integrated100msBlocksSinceReset = 0;
        momentaryLufs = -100.0f;
        shortTermLufs = -100.0f;
        integratedLufs = -100.0f;
        resetTruePeak();
    }

    void resetIntegratedAndTruePeak() noexcept
    {
        clearIntegratedHistogram();
        integrated100msBlocksSinceReset = 0;
        integratedLufs = -100.0f;
        resetTruePeak();
    }

    void processSample (float left, float right) noexcept
    {
        const std::array<float, 2> input { left, right };

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto weighted = channels[static_cast<std::size_t> (channel)].stage1.process (input[static_cast<std::size_t> (channel)]);
            weighted = channels[static_cast<std::size_t> (channel)].stage2.process (weighted);
            current100msSquares[static_cast<std::size_t> (channel)] += static_cast<double> (weighted) * weighted;
        }

        updateTruePeak (left, right);

        if (++samplesInCurrent100ms >= samplesPer100ms)
            finish100msBlock();
    }

    float getMomentaryLufs() const noexcept { return momentaryLufs; }
    float getShortTermLufs() const noexcept { return shortTermLufs; }
    float getIntegratedLufs() const noexcept { return integratedLufs; }
    float getMaxTruePeakDbTP() const noexcept { return maxTruePeakDbTP; }

private:
    class Biquad
    {
    public:
        void setFromAnalog (double sr,
                            double nb0, double nb1, double nb2,
                            double da0, double da1, double da2) noexcept
        {
            const auto k = 2.0 * std::max (1.0, sr);
            const auto k2 = k * k;

            const auto db0 = da0 * k2 + da1 * k + da2;
            const auto db1 = -2.0 * da0 * k2 + 2.0 * da2;
            const auto db2 = da0 * k2 - da1 * k + da2;

            const auto nbz0 = nb0 * k2 + nb1 * k + nb2;
            const auto nbz1 = -2.0 * nb0 * k2 + 2.0 * nb2;
            const auto nbz2 = nb0 * k2 - nb1 * k + nb2;

            const auto invA0 = 1.0 / std::max (1.0e-30, db0);
            b0 = static_cast<float> (nbz0 * invA0);
            b1 = static_cast<float> (nbz1 * invA0);
            b2 = static_cast<float> (nbz2 * invA0);
            a1 = static_cast<float> (db1 * invA0);
            a2 = static_cast<float> (db2 * invA0);
            reset();
        }

        void reset() noexcept { z1 = z2 = 0.0f; }

        float process (float x) noexcept
        {
            const auto y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            denormalizeBiquadState(z1, z2);
            return y;
        }

    private:
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
        float a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;
    };

    struct ChannelState
    {
        Biquad stage1;
        Biquad stage2;
    };

    struct EnergyBlock
    {
        std::array<double, 2> meanSquares {};
    };

    static float energyToLufs (double energy) noexcept
    {
        if (energy <= 1.0e-20)
            return -100.0f;
        return static_cast<float> (-0.691 + 10.0 * std::log10 (energy));
    }

    double summedEnergy (const EnergyBlock& block) const noexcept
    {
        double sum = block.meanSquares[0];
        if (numChannels > 1)
            sum += block.meanSquares[1];
        return sum;
    }

    void finish100msBlock() noexcept
    {
        EnergyBlock block;
        const auto denom = static_cast<double> (std::max (1, samplesInCurrent100ms));
        for (int channel = 0; channel < numChannels; ++channel)
            block.meanSquares[static_cast<std::size_t> (channel)] = current100msSquares[static_cast<std::size_t> (channel)] / denom;

        block100msHistory[static_cast<std::size_t> (blockHistoryWrite)] = block;
        blockHistoryWrite = (blockHistoryWrite + 1) % static_cast<int> (block100msHistory.size());
        blockHistoryCount = std::min (blockHistoryCount + 1, static_cast<int> (block100msHistory.size()));
        ++integrated100msBlocksSinceReset;

        current100msSquares.fill (0.0);
        samplesInCurrent100ms = 0;

        updateMomentary();
        updateShortTerm();
        updateIntegrated();
    }

    float recentLoudness (int requestedBlocks) const noexcept
    {
        const auto count = std::min (blockHistoryCount, requestedBlocks);
        if (count <= 0)
            return -100.0f;

        double energy = 0.0;
        for (int i = 0; i < count; ++i)
        {
            auto index = blockHistoryWrite - 1 - i;
            while (index < 0)
                index += static_cast<int> (block100msHistory.size());
            energy += summedEnergy (block100msHistory[static_cast<std::size_t> (index)]);
        }

        return energyToLufs (energy / static_cast<double> (count));
    }

    void updateMomentary() noexcept
    {
        momentaryLufs = recentLoudness (4);
    }

    void updateShortTerm() noexcept
    {
        shortTermLufs = recentLoudness (30);
    }

    void updateIntegrated() noexcept
    {
        // A reset starts a fresh integrated interval; wait for four new 100 ms
        // blocks before forming the first 400 ms gating block.
        if (integrated100msBlocksSinceReset < 4 || blockHistoryCount < 4)
            return;

        double energy400ms = 0.0;
        for (int i = 0; i < 4; ++i)
        {
            auto index = blockHistoryWrite - 1 - i;
            while (index < 0)
                index += static_cast<int> (block100msHistory.size());
            energy400ms += summedEnergy (block100msHistory[static_cast<std::size_t> (index)]);
        }
        energy400ms *= 0.25;

        const auto blockLufs = energyToLufs (energy400ms);
        if (blockLufs > absoluteGateLufs && ! integratedEnergyHistogram.empty())
        {
            const auto bin = binForLufs (blockLufs);
            integratedEnergyHistogram[bin] += energy400ms;
            ++integratedCountHistogram[bin];
            integratedTotalEnergy += energy400ms;
            ++integratedTotalCount;
        }

        if (integratedTotalCount == 0)
        {
            integratedLufs = -100.0f;
            return;
        }

        // Relative gate: -10 LU below the absolute-gated mean.
        const auto absoluteGatedLufs = energyToLufs (integratedTotalEnergy / static_cast<double> (integratedTotalCount));
        const auto finalThreshold = std::max (absoluteGateLufs, absoluteGatedLufs - 10.0f);

        double gatedEnergy = 0.0;
        std::uint64_t gatedCount = 0;
        for (std::size_t bin = 0; bin < integratedHistogramBins; ++bin)
        {
            if (integratedCountHistogram[bin] == 0)
                continue;

            // Blocks are compared with the threshold at the bin centre (0.005 LU resolution).
            const auto binCentreLufs = absoluteGateLufs + (static_cast<float> (bin) + 0.5f) * integratedHistogramStepLu;
            if (binCentreLufs > finalThreshold)
            {
                gatedEnergy += integratedEnergyHistogram[bin];
                gatedCount += integratedCountHistogram[bin];
            }
        }

        integratedLufs = gatedCount > 0
            ? energyToLufs (gatedEnergy / static_cast<double> (gatedCount))
            : -100.0f;
    }

    static std::size_t binForLufs (float lufs) noexcept
    {
        const auto position = (lufs - absoluteGateLufs) / integratedHistogramStepLu;
        const auto bin = static_cast<long long> (std::floor (position));
        return static_cast<std::size_t> (std::clamp<long long> (bin, 0, static_cast<long long> (integratedHistogramBins) - 1));
    }

    void clearIntegratedHistogram() noexcept
    {
        std::fill (integratedEnergyHistogram.begin(), integratedEnergyHistogram.end(), 0.0);
        std::fill (integratedCountHistogram.begin(), integratedCountHistogram.end(), 0);
        integratedTotalEnergy = 0.0;
        integratedTotalCount = 0;
    }

    void resetTruePeak() noexcept
    {
        for (auto& channel : truePeakHistory)
            channel.fill (0.0f);
        truePeakWriteIndex = 0;
        maxTruePeakDbTP = -100.0f;
    }

    void updateTruePeak (float left, float right) noexcept
    {
        truePeakHistory[0][static_cast<std::size_t> (truePeakWriteIndex)] = left;
        truePeakHistory[1][static_cast<std::size_t> (truePeakWriteIndex)] = right;

        float maximum = 0.0f;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            for (int phase = 0; phase < 4; ++phase)
            {
                float value = 0.0f;
                auto historyIndex = truePeakWriteIndex;
                for (int tap = 0; tap < 12; ++tap)
                {
                    value += truePeakHistory[static_cast<std::size_t> (channel)][static_cast<std::size_t> (historyIndex)]
                           * SantosTruePeakCoefficients::values[static_cast<std::size_t> (tap)][static_cast<std::size_t> (phase)];
                    if (--historyIndex < 0)
                        historyIndex = 11;
                }
                maximum = std::max (maximum, std::abs (value));
            }

            // The four interpolation phases of the BS.1770 filter never land exactly
            // on the original samples, so include the two samples the phases sit
            // between. This keeps true peak >= sample peak; without it the reading
            // could fall below the sample peak on high-frequency content.
            const auto& channelHistory = truePeakHistory[static_cast<std::size_t> (channel)];
            const auto alignedA = (truePeakWriteIndex - 5 + 12) % 12;
            const auto alignedB = (truePeakWriteIndex - 6 + 12) % 12;
            maximum = std::max (maximum, std::abs (channelHistory[static_cast<std::size_t> (alignedA)]));
            maximum = std::max (maximum, std::abs (channelHistory[static_cast<std::size_t> (alignedB)]));
        }

        if (++truePeakWriteIndex >= 12)
            truePeakWriteIndex = 0;

        if (maximum > 1.0e-8f)
            maxTruePeakDbTP = std::max (maxTruePeakDbTP, 20.0f * std::log10 (maximum));
    }

    double sampleRate = 48000.0;
    int numChannels = 2;
    int samplesPer100ms = 4800;
    int samplesInCurrent100ms = 0;

    std::array<ChannelState, 2> channels {};
    std::array<double, 2> current100msSquares {};
    std::array<EnergyBlock, 30> block100msHistory {};
    int blockHistoryWrite = 0;
    int blockHistoryCount = 0;
    int integrated100msBlocksSinceReset = 0;
    static constexpr float absoluteGateLufs = -70.0f;
    static constexpr float integratedHistogramStepLu = 0.01f;
    static constexpr std::size_t integratedHistogramBins = 8000; // -70 .. +10 LUFS
    std::vector<double> integratedEnergyHistogram;
    std::vector<std::uint64_t> integratedCountHistogram;
    double integratedTotalEnergy = 0.0;
    std::uint64_t integratedTotalCount = 0;

    float momentaryLufs = -100.0f;
    float shortTermLufs = -100.0f;
    float integratedLufs = -100.0f;

    std::array<std::array<float, 12>, 2> truePeakHistory {};
    int truePeakWriteIndex = 0;
    float maxTruePeakDbTP = -100.0f;
};
