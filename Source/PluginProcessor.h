#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

#include "HistoryBuffer.h"
#include "LevelerEngine.h"

class SantosLevelerAudioProcessor final : public juce::AudioProcessor
{
public:
    static constexpr std::size_t spectrumSize = 2048;

    SantosLevelerAudioProcessor();
    ~SantosLevelerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;

    const SantosHistoryBuffer& getHistory() const noexcept { return history; }
    float getInputMeterDb() const noexcept  { return inputMeterDb.load (std::memory_order_relaxed); }
    float getOutputMeterDb() const noexcept { return outputMeterDb.load (std::memory_order_relaxed); }
    float getRiderDb() const noexcept { return riderMeterDb.load (std::memory_order_relaxed); }
    float getPeakReductionDb() const noexcept { return peakMeterDb.load (std::memory_order_relaxed); }
    bool getRiderActive() const noexcept { return riderActive.load (std::memory_order_relaxed); }
    bool getTransportPlaying() const noexcept { return transportPlaying.load (std::memory_order_relaxed); }
    bool hasHostTransport() const noexcept { return hostTransportKnown.load (std::memory_order_relaxed); }
    double getCurrentSampleRateForDisplay() const noexcept { return currentSampleRate; }

    void copySpectrumInput (std::array<float, spectrumSize>& destination) const noexcept;

private:
    SantosLevelerEngine engine;
    SantosHistoryBuffer history;

    juce::AudioBuffer<float> lookaheadBuffer;

    double currentSampleRate = 48000.0;

    int lookaheadWritePosition = 0;
    int lookaheadBufferSize = 1;
    int maxLookaheadSamples = 0;
    int currentLookaheadSamples = 0;

    std::atomic<float> inputMeterDb { -100.0f };
    std::atomic<float> outputMeterDb { -100.0f };
    std::atomic<float> riderMeterDb { 0.0f };
    std::atomic<float> peakMeterDb { 0.0f };
    std::atomic<bool> riderActive { false };
    std::atomic<bool> transportPlaying { true };
    std::atomic<bool> hostTransportKnown { false };

    std::array<float, spectrumSize> spectrumInput {};
    std::atomic<std::uint32_t> spectrumWriteCount { 0 };

    int historyCounter = 0;
    int historyPeriodSamples = 800;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantosLevelerAudioProcessor)
};
