#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

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
        integrated400msEnergies.clear();
        integrated100msBlocksSinceReset = 0;
        integratedUpdateCounter = 0;
        shortTermLufs = -100.0f;
        integratedLufs = -100.0f;
        resetTruePeak();
    }

    void resetIntegratedAndTruePeak() noexcept
    {
        integrated400msEnergies.clear();
        integrated100msBlocksSinceReset = 0;
        integratedUpdateCounter = 0;
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

        updateShortTerm();
        updateIntegrated();
    }

    void updateShortTerm() noexcept
    {
        const auto count = std::min (blockHistoryCount, 30);
        if (count <= 0)
        {
            shortTermLufs = -100.0f;
            return;
        }

        double energy = 0.0;
        for (int i = 0; i < count; ++i)
        {
            auto index = blockHistoryWrite - 1 - i;
            while (index < 0)
                index += static_cast<int> (block100msHistory.size());
            energy += summedEnergy (block100msHistory[static_cast<std::size_t> (index)]);
        }

        shortTermLufs = energyToLufs (energy / static_cast<double> (count));
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

        if (energyToLufs (energy400ms) > -70.0f)
            integrated400msEnergies.push_back (energy400ms);

        // EBU Mode only requires the live Integrated display to update at 1 Hz.
        // Keep the 100 ms gating blocks, but avoid re-scanning the full history 10x/sec.
        if (++integratedUpdateCounter < 10 && integrated400msEnergies.size() > 1)
            return;
        integratedUpdateCounter = 0;

        if (integrated400msEnergies.empty())
        {
            integratedLufs = -100.0f;
            return;
        }

        double absoluteEnergy = 0.0;
        for (const auto energy : integrated400msEnergies)
            absoluteEnergy += energy;
        absoluteEnergy /= static_cast<double> (integrated400msEnergies.size());

        const auto relativeThreshold = energyToLufs (absoluteEnergy) - 10.0f;
        const auto finalThreshold = std::max (-70.0f, relativeThreshold);

        double gatedEnergy = 0.0;
        std::size_t gatedCount = 0;
        for (const auto energy : integrated400msEnergies)
        {
            if (energyToLufs (energy) > finalThreshold)
            {
                gatedEnergy += energy;
                ++gatedCount;
            }
        }

        integratedLufs = gatedCount > 0
            ? energyToLufs (gatedEnergy / static_cast<double> (gatedCount))
            : -100.0f;
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
                           * truePeakCoefficients[static_cast<std::size_t> (tap)][static_cast<std::size_t> (phase)];
                    if (--historyIndex < 0)
                        historyIndex = 11;
                }
                maximum = std::max (maximum, std::abs (value));
            }
        }

        if (++truePeakWriteIndex >= 12)
            truePeakWriteIndex = 0;

        if (maximum > 1.0e-8f)
            maxTruePeakDbTP = std::max (maxTruePeakDbTP, 20.0f * std::log10 (maximum));
    }

    static constexpr std::array<std::array<float, 4>, 12> truePeakCoefficients {{
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
    int samplesPer100ms = 4800;
    int samplesInCurrent100ms = 0;

    std::array<ChannelState, 2> channels {};
    std::array<double, 2> current100msSquares {};
    std::array<EnergyBlock, 30> block100msHistory {};
    int blockHistoryWrite = 0;
    int blockHistoryCount = 0;
    int integrated100msBlocksSinceReset = 0;
    int integratedUpdateCounter = 0;
    std::vector<double> integrated400msEnergies;

    float shortTermLufs = -100.0f;
    float integratedLufs = -100.0f;

    std::array<std::array<float, 12>, 2> truePeakHistory {};
    int truePeakWriteIndex = 0;
    float maxTruePeakDbTP = -100.0f;
};
