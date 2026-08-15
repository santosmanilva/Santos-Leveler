#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

#include "HistoryBuffer.h"
#include "LevelerEngine.h"
#include "TruePeakLimiter.h"

class SantosLevelerAudioProcessor final : public juce::AudioProcessor
{
public:
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
    bool getRiderActive() const noexcept { return riderActive.load (std::memory_order_relaxed); }
    bool getTransportPlaying() const noexcept { return transportPlaying.load (std::memory_order_relaxed); }
    bool hasHostTransport() const noexcept { return hostTransportKnown.load (std::memory_order_relaxed); }
    float getTruePeakReductionDb() const noexcept { return truePeakLimiter.getGainReductionDb(); }
    float getDetectedTruePeakDbTP() const noexcept { return truePeakLimiter.getDetectedTruePeakDbTP(); }

    void ensureABStatesInitialised();
    void selectABState (bool useB);
    bool isABStateB() const noexcept { return abStateBSelected.load (std::memory_order_relaxed); }

private:
    using ABState = std::array<float, 11>;

    ABState captureCurrentABState() const;
    void applyABState (const ABState& state);

    SantosLevelerEngine engine;
    SantosTruePeakLimiter truePeakLimiter;
    SantosHistoryBuffer history;

    juce::AudioBuffer<float> lookaheadBuffer;
    std::vector<float> historyInputDbBuffer;

    double currentSampleRate = 48000.0;

    int lookaheadWritePosition = 0;
    int lookaheadBufferSize = 1;
    int maxLookaheadSamples = 0;
    int currentLookaheadSamples = 0;
    int targetLookaheadSamples = 0;
    int lookaheadTransitionLengthSamples = 1;
    int lookaheadTransitionSamplesRemaining = 0;

    float bypassMix = 0.0f;
    float bypassSmoothingAlpha = 1.0f;

    juce::CriticalSection abStateLock;
    ABState abStateA {};
    ABState abStateB {};
    bool abStatesInitialised = false;
    std::atomic<bool> abStateBSelected { false };

    std::atomic<float> inputMeterDb { -100.0f };
    std::atomic<float> outputMeterDb { -100.0f };
    std::atomic<float> riderMeterDb { 0.0f };
    std::atomic<bool> riderActive { false };
    std::atomic<bool> transportPlaying { true };
    std::atomic<bool> hostTransportKnown { false };

    int historyCounter = 0;
    int historyPeriodSamples = 800;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SantosLevelerAudioProcessor)
};
